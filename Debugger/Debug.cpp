
#include "Inc\Debug.h"
#include "Inc\DebugHelpers.h"

extern PLOGGER pstLogger;
extern HINSTANCE g_hMainInstance;

typedef struct _TargetInfo {
    PDEBUGINFO pstDebugInfoFromGui;

    DWORD dwPID;
    DWORD dwMainThreadID;
    
    CREATE_PROCESS_DEBUG_INFO stProcessInfo;

    CHL_HTABLE *phtDllsLoaded;
    CHL_HTABLE *phtThreads;
    
    int nCurThreads;
    int nTotalProcesses;
    int nTotalThreads;

    int nCurDllsLoaded;
    int nTotalDllsLoaded;

    LPDEBUG_EVENT lpDebugEvent;
}TARGETINFO, *PTARGETINFO;

static BOOL fDebugNewProgram(PTARGETINFO pstTargetInfo);
static BOOL fDebugActiveProcess(PTARGETINFO pstTargetInfo);
static void vOnThisThreadExit(PTARGETINFO *ppstTargetInfo);
static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
static BOOL fProcessDebugEventLoop(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);

// Debug event handlers
BOOL fOnCreateProcess(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
BOOL fOnCreateThread(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
BOOL fOnLoadDll(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
BOOL fOnUnloadDll(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
BOOL fOnExitThread(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
BOOL fOnExitProcess(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
BOOL fOnOutputDebugString(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
BOOL fOnException(PTARGETINFO pstTargetInfo, __out DWORD *pdwContinueStatus, __out DWORD *pdwErrCode);
// BOOL fOnRIP(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);

// Entry point to the debug thread
DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs)
{
    ASSERT(lpvArgs);

    DWORD dwErrCode = ERROR_SUCCESS;

    PTARGETINFO pstTargetInfo = NULL;

    HANDLE hInitSyncEvent = NULL;
    BOOL fProcessingGuiMessages;
    BOOL fProcessingDebugEventLoop;

    vWriteLog(pstLogger, L"%s(): Entry", __FUNCTIONW__);

    if(!fChlMmAlloc((void**)&pstTargetInfo, sizeof(TARGETINFO), &dwErrCode))
    {
        vWriteLog(pstLogger, L"%s(): fChlMmAlloc() failed: %u", __FUNCTIONW__, dwErrCode);

        // todo: signal sync event
        goto error_return;
    }

    pstTargetInfo->pstDebugInfoFromGui = (PDEBUGINFO)lpvArgs;

    // New program OR active process? - work on it
    if(pstTargetInfo->pstDebugInfoFromGui->fDebuggingActiveProcess)
    {
        if(!fDebugActiveProcess(pstTargetInfo))
        {
            // todo:
        }
    }
    else
    {
        if(!fDebugNewProgram(pstTargetInfo))
        {
            // todo:
        }
    }

    // Force system to create message queue for this thread
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Signal init sync event to Gui thread
    ASSERT(pstTargetInfo->pstDebugInfoFromGui->pszInitSyncEvtName);
    if( (hInitSyncEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, pstTargetInfo->pstDebugInfoFromGui->pszInitSyncEvtName)) == NULL )
    {
        SET_ERRORCODE(dwErrCode);
        vWriteLog(pstLogger, L"%s(): OpenEvent() failed %u", __FUNCTIONW__, dwErrCode);
        goto error_return;
    }

    if(!SetEvent(hInitSyncEvent))
    {
        SET_ERRORCODE(dwErrCode);
        ASSERT(FALSE);
        // todo: handle
    }
    FREE_HANDLE(hInitSyncEvent);
    pstTargetInfo->pstDebugInfoFromGui->pszInitSyncEvtName = NULL;

    // Start from-gui message loop and debug event loop
    fProcessingGuiMessages = fProcessingDebugEventLoop = TRUE;

    // Execute loop while we have either GUI messages or debug event loop to process
    while(fProcessingGuiMessages || fProcessingDebugEventLoop)
    {
        if(fProcessingGuiMessages && !fProcessGuiMessage(pstTargetInfo, &dwErrCode))
        {
            fProcessingGuiMessages = FALSE;
        }

        if(fProcessingDebugEventLoop && !fProcessDebugEventLoop(pstTargetInfo, &dwErrCode))
        {
            fProcessingDebugEventLoop = FALSE;
        }
    }

    vWriteLog(pstLogger, L"%s(): Exiting with ERROR_SUCCESS", __FUNCTIONW__);
    vOnThisThreadExit(&pstTargetInfo);
    return ERROR_SUCCESS;

    error_return:
    vWriteLog(pstLogger, L"%s(): Exiting with error code %u", __FUNCTIONW__, dwErrCode);
    vOnThisThreadExit(&pstTargetInfo);
    return dwErrCode;
}

static BOOL fDebugNewProgram(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->pstDebugInfoFromGui->pszTargetPath);

    STARTUPINFO StartUpInfo;
    PROCESS_INFORMATION ProcInfo;

    ZeroMemory(&StartUpInfo, sizeof(StartUpInfo));
    StartUpInfo.cb = sizeof(StartUpInfo);

    // Create the process first
    if(!CreateProcess(  pstTargetInfo->pstDebugInfoFromGui->pszTargetPath,    // app name
                        NULL,               // command line
                        NULL,               // process attr
                        NULL,               // thread attr
                        FALSE,              // inherit handles
                        DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,    // creation flags
                        NULL,               // environment
                        NULL,               // cur directory
                        &StartUpInfo,
                        &ProcInfo))
    {
        logerror(pstLogger, L"Unable to create debuggee process: %d", GetLastError());
        goto error_return;
    }

    // Close these handles because we will get them in debug event loop
    FREE_HANDLE(ProcInfo.hProcess);
    FREE_HANDLE(ProcInfo.hThread);

    // Set up required data structures before starting debugging loop
    pstTargetInfo->dwPID = ProcInfo.dwProcessId;
    pstTargetInfo->dwMainThreadID = ProcInfo.dwThreadId;

    // Create the hashtable for thread info
    if(!fChlDsCreateHT(&(pstTargetInfo->phtThreads), iChlDsGetNearestTableSizeIndex(100), HT_KEY_DWORD, HT_VAL_STR))
    {
        logerror(pstLogger, L"fChlDsCreateHT() returned FALSE");
        goto error_return;
    }
    return TRUE;

    error_return:
    return FALSE;
}

static BOOL fDebugActiveProcess(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);

    // Set up required data structures before starting debugging loop
    return FALSE;
}

static void vOnThisThreadExit(PTARGETINFO *ppstTargetInfo)
{
    // ppstTargetInfo will never be NULL because it is a stack variable.
    // It is a pointer to a pointer variable.
    // We must make sure that the pointer to a pointer has a non-NULL destination
    ASSERT(*ppstTargetInfo);

    PTARGETINFO plocal = *ppstTargetInfo;

    // free threads table
    if(plocal->phtThreads && !fChlDsDestroyHT(plocal->phtThreads))
    {
        logerror(pstLogger, L"(%s): Unable to destroy DLL hash table", __FUNCTIONW__);
    }

    // free Dlls table
    if(plocal->phtDllsLoaded && !fChlDsDestroyHT(plocal->phtDllsLoaded))
    {
        logerror(pstLogger, L"(%s): Unable to destroy DLL hash table", __FUNCTIONW__);
    }

    // free target path
    if(!plocal->pstDebugInfoFromGui->fDebuggingActiveProcess)
    {
        ASSERT(plocal->pstDebugInfoFromGui->pszTargetPath);
        vChlMmFree((void**)&plocal->pstDebugInfoFromGui->pszTargetPath);
    }
    else
    {
        ASSERT(plocal->pstDebugInfoFromGui->pszTargetPath == NULL);
    }

    // Free the memory occupied by DEBUGINFO
    vChlMmFree((void**)&plocal->pstDebugInfoFromGui);

    // Finally, free the memory occupied by TARGETINFO
    vChlMmFree((void**)&plocal);

    return;
}

static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);

    IFPTR_SETVAL(pdwErrCode, ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

static BOOL fProcessDebugEventLoop(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);

    DEBUG_EVENT de;

    BOOL fRetVal;
    DWORD dwErrCode;

    //logtrace("Debug thread: Waiting for debug event...");

    fRetVal = TRUE;
    if(!WaitForDebugEvent(&de, 10))
    {
        SET_ERRORCODE(dwErrCode);
        dbgwprintf(L"WaitForDebugEvent() failed: %u", dwErrCode);
        logwarn(pstLogger, L"WaitForDebugEvent() failed: %u", dwErrCode);
    }
    else
    {
        switch(de.dwDebugEventCode)
        {
            case EXCEPTION_DEBUG_EVENT:
            {
                DWORD dwContinueStatus;

                fOnException(pstTargetInfo, &dwContinueStatus, NULL);
                ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);
                
                return TRUE;
            }

            case CREATE_PROCESS_DEBUG_EVENT:
            {
                fOnCreateProcess(pstTargetInfo, &dwErrCode);
                break;
            }

            case CREATE_THREAD_DEBUG_EVENT:
            {
                fOnCreateThread(pstTargetInfo, &dwErrCode);
                break;
            }

            case LOAD_DLL_DEBUG_EVENT:
            {
                fOnLoadDll(pstTargetInfo, &dwErrCode);
                break;
            }

            case UNLOAD_DLL_DEBUG_EVENT:
            {
                fOnUnloadDll(pstTargetInfo, &dwErrCode);
                break;
            }

            case EXIT_THREAD_DEBUG_EVENT:
            {
                fOnExitThread(pstTargetInfo, &dwErrCode);
                break;
            }

            case EXIT_PROCESS_DEBUG_EVENT:
            {
                fOnExitProcess(pstTargetInfo, &dwErrCode);
                break;
            }

            case OUTPUT_DEBUG_STRING_EVENT:
            {
                fOnOutputDebugString(pstTargetInfo, &dwErrCode);
                break;
            }

            case RIP_EVENT:
            {
                IFPTR_SETVAL(pdwErrCode, ERROR_CALL_NOT_IMPLEMENTED);
                fRetVal = FALSE;
                break;
            }

            default:
            {
                dbgwprintf(L"%s(): Unexpected debug event %d\n", __FUNCTIONW__, de.dwDebugEventCode);
                logerror(pstLogger, L"%s(): Unexpected debug event %d\n", __FUNCTIONW__, de.dwDebugEventCode);
                break;
            }

        }// switch(de.dwDebugEventCode)

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);

    }// if-else

    return fRetVal;
}

// Handles the CREATE_PROCESS_DEBUG_EVENT.
// Increments the thread and process counters,
// Copies process info into the target info structure.
// Return: TRUE always
BOOL fOnCreateProcess(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

#ifdef _DEBUG
    WCHAR wsImageName[SLEN_MAXPATH];
    DWORD dwRet = GetFinalPathNameByHandle(lpDebugEvent->u.CreateProcessInfo.hFile, wsImageName, SLEN_MAXPATH - 1, VOLUME_NAME_NT);
    wsImageName[SLEN_MAXPATH - 1] = 0;

    if(dwRet == 0)
        logerror(pstLogger, L"Could not get filename from handle");
    else if(dwRet > MAX_PATH)
        logerror(pstLogger, L"Insufficient buffer size for GetFinalPathNameByHandle()");
    else
    {
        wprintf(L"Process create: [%u] : %s", lpDebugEvent->dwProcessId, wsImageName);
    }
#endif

    ++(pstTargetInfo->nCurThreads);
    ++(pstTargetInfo->nTotalThreads);
    ++(pstTargetInfo->nTotalProcesses);

    memcpy(&(pstTargetInfo->stProcessInfo), &(lpDebugEvent->u.CreateProcessInfo), sizeof(CREATE_PROCESS_DEBUG_EVENT));

    // Close handle to the image file, we do not need here... yet
    CloseHandle(lpDebugEvent->u.CreateProcessInfo.hFile);

    return TRUE;
}

BOOL fOnCreateThread(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    dbgwprintf(L"Thread create: [%u]", lpDebugEvent->dwThreadId);
    if(!fAddThread(pstTargetInfo->phtThreads, lpDebugEvent->dwThreadId, &(lpDebugEvent->u.CreateThread)))
    {
        logerror(pstLogger, L"Unable to insert thread to hashtable");
    }

    ++(pstTargetInfo->nTotalThreads);
    ++(pstTargetInfo->nCurThreads);

    return TRUE;
}

BOOL fOnLoadDll(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    DWORD dwRet;
    WCHAR wsImageName[SLEN_MAXPATH];
    
    dwRet = GetFinalPathNameByHandle(lpDebugEvent->u.LoadDll.hFile, wsImageName, SLEN_MAXPATH - 1, VOLUME_NAME_NT);
    wsImageName[SLEN_MAXPATH - 1] = 0;

    if(dwRet == 0)
        logerror(pstLogger, L"Could not get filename from handle");
    else if(dwRet > MAX_PATH)
        logerror(pstLogger, L"Insufficient buffer size for GetFinalPathNameByHandle()");
    else
    {
        wprintf(L"DLL Load: [0x%08x] : %s", lpDebugEvent->u.LoadDll.lpBaseOfDll, wsImageName);
    }

    // Insert into the DLLs loaded hashtable
    if(!fChlDsInsertHT(pstTargetInfo->phtDllsLoaded, &(lpDebugEvent->u.LoadDll.lpBaseOfDll), sizeof(DWORD), wsImageName, 
        CONV_BYTES_wcsnlen_s(wsImageName, SLEN_MAXPATH) ))
    {
        logerror(pstLogger, L"Unable to insert 0x%08x:%s into hash", *((DWORD*)lpDebugEvent->u.LoadDll.lpBaseOfDll), wsImageName);
    }

    ++(pstTargetInfo->nCurDllsLoaded);
    ++(pstTargetInfo->nTotalDllsLoaded);

    CloseHandle(lpDebugEvent->u.LoadDll.hFile);

    return TRUE;
}

BOOL fOnUnloadDll(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    WCHAR *pws = NULL;
    int outValSize = 0;

    if(!fChlDsFindHT(pstTargetInfo->phtDllsLoaded, &(lpDebugEvent->u.UnloadDll.lpBaseOfDll), sizeof(DWORD), &pws, &outValSize))
    {
        wprintf(L"DLL Unload : [0x%08x] : Image name not found", (DWORD)(lpDebugEvent->u.UnloadDll.lpBaseOfDll));
    }
    else
    {
        wprintf(L"DLL Unload : [0x%08x] : %s", (DWORD)lpDebugEvent->u.UnloadDll.lpBaseOfDll, pws);
        fChlDsRemoveHT(pstTargetInfo->phtDllsLoaded, &(lpDebugEvent->u.UnloadDll.lpBaseOfDll), sizeof(DWORD));
    }

    --(pstTargetInfo->nCurDllsLoaded);

    return TRUE;
}

BOOL fOnExitThread(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    dbgwprintf(L"Thread exit: [%u] : ExitCode %d", lpDebugEvent->dwThreadId, lpDebugEvent->u.ExitThread.dwExitCode);
    if(!fRemoveThread(pstTargetInfo->phtThreads, lpDebugEvent->dwThreadId, NULL))
    {
        logerror(pstLogger, L"Failed to remove thread from hashtable");
    }

    --(pstTargetInfo->nCurThreads);

    return TRUE;
}

BOOL fOnExitProcess(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    BOOL fRetVal = TRUE;
    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    wprintf(L"Process exit: [%u] : ExitCode %d\r\n", lpDebugEvent->dwProcessId, lpDebugEvent->u.ExitProcess.dwExitCode);
    if(lpDebugEvent->dwProcessId == pstTargetInfo->dwPID)
    {
        wprintf(
            L"Debuggee process EXIT! Gotta quit debug thread...\r\n"
            L"Processes created    : %u\r\n"
            L"Threads created      : %u\r\n"
            L"DLLs loaded          : %u\r\n",
            pstTargetInfo->nTotalProcesses, 
            pstTargetInfo->nTotalThreads, 
            pstTargetInfo->nTotalDllsLoaded);
        
        fRetVal = FALSE;
    }

    return fRetVal;
}

BOOL fOnOutputDebugString(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    WCHAR *pwsDebugString = NULL;
    SIZE_T bytesRead = 0;

    if(!fChlMmAlloc((void**)&pwsDebugString, lpDebugEvent->u.DebugString.nDebugStringLength * sizeof(WCHAR), NULL))
    {
        logerror(pstLogger, L"OUTPUT_DEBUG_STRING_EVENT: Could not allocate memory!!");
    }
    else
    {
        // read the string from debuggee's memory
        if(lpDebugEvent->u.DebugString.fUnicode)
        {
            ReadProcessMemory(
                pstTargetInfo->stProcessInfo.hProcess, 
                lpDebugEvent->u.DebugString.lpDebugStringData, 
                pwsDebugString,
                lpDebugEvent->u.DebugString.nDebugStringLength * sizeof(WCHAR), 
                &bytesRead);

            wprintf(L"OutputDebugString: [%u][%s]", bytesRead, pwsDebugString);
        }
        else
        {
            ReadProcessMemory(
                pstTargetInfo->stProcessInfo.hProcess, 
                lpDebugEvent->u.DebugString.lpDebugStringData, 
                pwsDebugString,
                lpDebugEvent->u.DebugString.nDebugStringLength * sizeof(char), 
                &bytesRead);

            wprintf(L"OutputDebugString: [%u][%S]", bytesRead, pwsDebugString);
        }
        vChlMmFree((void**)&pwsDebugString);
    }

    return TRUE;
}

BOOL fOnException(PTARGETINFO pstTargetInfo, __out DWORD *pdwContinueStatus, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    DWORD dwExCode = lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode;
    WCHAR wsExString[SLEN_EXCEPTION_NAME];
    WCHAR wsExceptionMessage[SLEN_COMMON64];

    wprintf(
        L"Exception 0x%08X at 0x%08x", 
        lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode, 
        lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);

    if(!fGetExceptionName(dwExCode, wsExString, _countof(wsExString)))
    {
        wsExString[0] = 0;
        logwarn(pstLogger, L"Could not get exception name for exception code %x", lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode);
    }
    else
    {
        if(lpDebugEvent->u.Exception.dwFirstChance)
        {
            swprintf_s(
                wsExceptionMessage, 
                _countof(wsExceptionMessage), 
                L"%s Exception(FirstChance) at: 0x%08x", 
                wsExString, 
                lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);
        }
        else
        {
            swprintf_s(
                wsExceptionMessage, 
                _countof(wsExceptionMessage), 
                L"%s Exception at: 0x%08x", 
                wsExString, 
                lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);
        }
    }

    MessageBox(NULL, wsExceptionMessage, L"Exception Raised", MB_ICONSTOP|MB_OK);
    
    IFPTR_SETVAL(pdwContinueStatus, DBG_EXCEPTION_NOT_HANDLED);
    return TRUE;
}
