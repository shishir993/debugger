
#include "Inc\Debug.h"
#include "Inc\DebugHelpers.h"

#define STATE_DBG_INVALID       0
#define STATE_DBG_RUNNING       1
#define STATE_DBG_DEBUGGING     2

extern PLOGGER pstLogger;
extern HINSTANCE g_hMainInstance;

typedef struct _TargetInfo {
    PDEBUGINFO pstDebugInfoFromGui;

    BOOL fCreateProcessEventRecvd;

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

    int iDebugState;

    LPDEBUG_EVENT lpDebugEvent;
}TARGETINFO, *PTARGETINFO;

// ** File local functions **
static BOOL fInitialSyncWithGuiThread(const WCHAR *pszSyncEventName);
static BOOL fDebugNewProgram(PTARGETINFO pstTargetInfo);
static BOOL fDebugActiveProcess(PTARGETINFO pstTargetInfo);
static void vOnThisThreadExit(PTARGETINFO *ppstTargetInfo);
static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo);
static BOOL fProcessDebugEventLoop(PTARGETINFO pstTargetInfo);
static void vSetMenuItemsState(PTARGETINFO pstTargetInfo);

// Debug event handlers
BOOL fOnException(PTARGETINFO pstTargetInfo, __out DWORD *pdwContinueStatus);
BOOL fOnCreateThread(PTARGETINFO pstTargetInfo);
BOOL fOnCreateProcess(PTARGETINFO pstTargetInfo);
BOOL fOnExitThread(PTARGETINFO pstTargetInfo);
BOOL fOnExitProcess(PTARGETINFO pstTargetInfo);
BOOL fOnLoadDll(PTARGETINFO pstTargetInfo);
BOOL fOnUnloadDll(PTARGETINFO pstTargetInfo);
BOOL fOnOutputDebugString(PTARGETINFO pstTargetInfo);
BOOL fOnRIP(PTARGETINFO pstTargetInfo);

// Entry point to the debug thread
DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs)
{
    ASSERT(lpvArgs);

    DWORD dwErrCode = ERROR_SUCCESS;

    PTARGETINFO pstTargetInfo = NULL;

    BOOL fContinueProcessing;

    vWriteLog(pstLogger, L"%s(): Entry", __FUNCTIONW__);

    // Reserve memory for holding all information while debugging
    if(!fChlMmAlloc((void**)&pstTargetInfo, sizeof(TARGETINFO), &dwErrCode))
    {
        vWriteLog(pstLogger, L"%s(): fChlMmAlloc() failed: %u", __FUNCTIONW__, dwErrCode);
        vChlMmFree(&lpvArgs);
        return dwErrCode;
    }

    pstTargetInfo->pstDebugInfoFromGui = PDEBUGINFO(lpvArgs);

    if(!fInitialSyncWithGuiThread(PDEBUGINFO(lpvArgs)->szInitSyncEvtName))
    {
        dwErrCode = GetLastError();
        logerror(pstLogger, L"%s(): Could not sync with Gui thread: %u", dwErrCode);
        goto error_return;
    }

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

    pstTargetInfo->iDebugState = STATE_DBG_RUNNING;
    vSetMenuItemsState(pstTargetInfo);

    // Start from-gui message loop and debug event loop
    fContinueProcessing = TRUE;

    // Execute loop while we have either GUI messages or debug event loop to process
    while(fContinueProcessing)
    {
        fContinueProcessing = fProcessGuiMessage(pstTargetInfo);

        if(fContinueProcessing)
        {
            fContinueProcessing = fProcessDebugEventLoop(pstTargetInfo);
        }
    }

    vWriteLog(pstLogger, L"%s(): Exiting with ERROR_SUCCESS", __FUNCTIONW__);

    // TODO: notify Gui thread and update UI

    vOnThisThreadExit(&pstTargetInfo);
    return ERROR_SUCCESS;

    error_return:
    vWriteLog(pstLogger, L"%s(): Exiting with error code %u", __FUNCTIONW__, dwErrCode);
    vOnThisThreadExit(&pstTargetInfo);
    return dwErrCode;
}

static BOOL fInitialSyncWithGuiThread(const WCHAR *pszSyncEventName)
{
    ASSERT(pszSyncEventName);

    MSG msg;
    HANDLE hInitSyncEvent;

    // Force system to create message queue for this thread
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Signal init sync event to Gui thread
    if( (hInitSyncEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, pszSyncEventName)) == NULL )
    {
        vWriteLog(pstLogger, L"%s(): OpenEvent() failed %u", __FUNCTIONW__, GetLastError());
        return FALSE;
    }

    if(!SetEvent(hInitSyncEvent))
    {
        ASSERT(FALSE);
        // todo: handle
    }

    FREE_HANDLE(hInitSyncEvent);
    return TRUE;
}

static BOOL fDebugNewProgram(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);

    STARTUPINFO StartUpInfo;
    PROCESS_INFORMATION ProcInfo;

    ZeroMemory(&StartUpInfo, sizeof(StartUpInfo));
    StartUpInfo.cb = sizeof(StartUpInfo);

    // Create the process first
    if(!CreateProcess(  pstTargetInfo->pstDebugInfoFromGui->szTargetPath,    // app name
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

    // Copy required data values
    pstTargetInfo->dwPID = ProcInfo.dwProcessId;
    pstTargetInfo->dwMainThreadID = ProcInfo.dwThreadId;

    // Create the Threads and Dlls hashtable
    if(!fChlDsCreateHT(&pstTargetInfo->phtThreads, 100, HT_KEY_DWORD, HT_VAL_PTR, TRUE))
    {
        logerror(pstLogger, L"fChlDsCreateHT() failed");
        goto error_return;
    }

    if(!fChlDsCreateHT(&pstTargetInfo->phtDllsLoaded, 500, HT_KEY_DWORD, HT_VAL_STR, TRUE))
    {
        logerror(pstLogger, L"fChlDsCreateHT() failed");
        goto error_return;
    }

    return TRUE;

    error_return:
    return FALSE;
}

static BOOL fDebugActiveProcess(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->pstDebugInfoFromGui->fDebuggingActiveProcess);
    ASSERT(pstTargetInfo->pstDebugInfoFromGui->dwProcessID);

    DEBUG_EVENT de;

    pstTargetInfo->dwPID = pstTargetInfo->pstDebugInfoFromGui->dwProcessID;
    
    // Attach to the process
    dbgwprintf(L"Attaching to PID %u\n", pstTargetInfo->dwPID);
    if(!DebugActiveProcess(pstTargetInfo->dwPID))
    {
        logerror(pstLogger, L"DebugActiveProcess() failed %u", GetLastError());
        goto error_return;
    }

    // Create the Threads and Dlls hashtable
    if(!fChlDsCreateHT(&pstTargetInfo->phtThreads, 100, HT_KEY_DWORD, HT_VAL_PTR, TRUE))
    {
        logerror(pstLogger, L"fChlDsCreateHT() failed");
        goto error_return;
    }

    if(!fChlDsCreateHT(&pstTargetInfo->phtDllsLoaded, 500, HT_KEY_DWORD, HT_VAL_STR, TRUE))
    {
        logerror(pstLogger, L"fChlDsCreateHT() failed");
        goto error_return;
    }

    // Now, we start the debug event loop until we receive a EXCEPTION_DEBUG_EVENT
    while(TRUE)
    {
        if(WaitForDebugEvent(&de, 10))
        {
            if(de.dwProcessId != pstTargetInfo->dwPID)
            {
                logtrace(pstLogger, L"Debug thread: Received debug event for sibling process %u != %u", de.dwProcessId, pstTargetInfo->dwPID);
                dbgwprintf(L"Debug thread: Received debug event for sibling process %u != %u", de.dwProcessId, pstTargetInfo->dwPID);
            }
            else
            {
                pstTargetInfo->lpDebugEvent = &de;

                switch(de.dwDebugEventCode)
                {
                    case EXCEPTION_DEBUG_EVENT:
                    {
                        logtrace(pstLogger, L"Received EXCEPTION_DEBUG_EVENT while attaching to active process...");
                        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
                        return TRUE;
                    }

                    case CREATE_THREAD_DEBUG_EVENT:
                    {
                        fOnCreateThread(pstTargetInfo);
                        break;
                    }

                    case CREATE_PROCESS_DEBUG_EVENT:
                    {
                        fOnCreateProcess(pstTargetInfo);
                        break;
                    }

                    case LOAD_DLL_DEBUG_EVENT:
                    {
                        fOnLoadDll(pstTargetInfo);
                        break;
                    }

                    case EXIT_THREAD_DEBUG_EVENT:
                    case EXIT_PROCESS_DEBUG_EVENT:
                    case UNLOAD_DLL_DEBUG_EVENT:
                    case OUTPUT_DEBUG_STRING_EVENT:
                    case RIP_EVENT:
                    {
                        ASSERT(FALSE);
                        break;
                    }

                    default:
                    {
                        dbgwprintf(L"%s(): Unexpected debug event %d\n", __FUNCTIONW__, de.dwDebugEventCode);
                        logerror(pstLogger, L"%s(): Unexpected debug event %d\n", __FUNCTIONW__, de.dwDebugEventCode);
                        break;
                    }

                }// switch(de.dwDebugEventCode)
            }

            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);

        }// if-else
    }// while(TRUE)

    // Unreachable code
    return TRUE;

    error_return:
    return FALSE;
}

static void vOnThisThreadExit(PTARGETINFO *ppstTargetInfo)
{
    // ppstTargetInfo will never be NULL because it is a stack variable.
    // It is a pointer to a pointer variable.
    // We must make sure that the pointer to a pointer has a non-NULL destination
    ASSERT(*ppstTargetInfo);

    PTARGETINFO plocal = *ppstTargetInfo;
    PGUIDBGCOMM pstGuiComm = NULL;

    DWORD dwError = ERROR_SUCCESS;

    // Notify Gui thread of exit
    if(!fChlMmAlloc((void**)&pstGuiComm, sizeof(GUIDBGCOMM), &dwError))
    {
        logerror(pstLogger, L"%s(): fChlMmAlloc() failed %u", dwError);
    }
    else
    {
        pstGuiComm->dwThreadID = GetCurrentThreadId();
        memcpy(&pstGuiComm->stTabPageInfo, &plocal->pstDebugInfoFromGui->stTabPageInfo, sizeof(TABPAGEINFO));
        SendMessage(plocal->pstDebugInfoFromGui->hMainWindow, DG_SESS_TERM, (WPARAM)plocal->pstDebugInfoFromGui->stTabPageInfo.iTabIndex, (LPARAM)pstGuiComm);
    }

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

    // Free the memory occupied by DEBUGINFO
    vChlMmFree((void**)&plocal->pstDebugInfoFromGui);

    // Finally, free the memory occupied by TARGETINFO
    vChlMmFree((void**)&plocal);

    return;
}

// Process any messages posted to this thread by the Gui thread
// If this function returns FALSE, then it means that this Debug thread
// must exit. Right now, FALSE is returned only when detaching from target
static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);

    MSG msg;

    while( PeekMessage(&msg, (HWND)-1, CUSTOM_GDEVENT_START, CUSTOM_GDEVENT_END, PM_REMOVE) )
    {
        switch(msg.message)
        {
            case GD_TAB_INFOCUS:
            {
                vSetMenuItemsState(pstTargetInfo);
                break;
            }

            case GD_TAB_OUTFOCUS:
            {
                // todo: nothing as of now
                break;
            }

            case GD_MENU_CONTINUE:
            {
                break;
            }

            case GD_MENU_STEPINTO:
            {
                break;
            }

            case GD_MENU_STEPOVER:
            {
                break;
            }

            case GD_MENU_STEPOUT:
            {
                break;
            }

            case GD_MENU_BREAKALL:
            {
                // todo: break all
                // set state to debugging
                // modify menu items state
                break;
            }

            case GD_MENU_SUSPALL:
            {
                if(!fSuspendAllThreads(pstTargetInfo->phtThreads))
                {
                    MessageBox(NULL, L"Error suspending threads. See log file.", L"Error", MB_ICONWARNING);
                }
                break;
            }

            case GD_MENU_RESALL:
            {
                if(!fResumeAllThreads(pstTargetInfo->phtThreads))
                {
                    MessageBox(NULL, L"Error resuming threads. See log file.", L"Error", MB_ICONWARNING);
                }
                break;
            }

            case GD_MENU_SUSPRES:
            {
                break;
            }

            case GD_SESS_TERM:
            {
                if(!TerminateProcess(pstTargetInfo->stProcessInfo.hProcess, EXITCODE_TARGET_TERM))
                {
                    MessageBox(pstTargetInfo->pstDebugInfoFromGui->hMainWindow, L"Cannot terminate target. See log file.", L"Error", MB_ICONERROR);
                    logerror(pstLogger, L"%s(): TerminateProcess() failed %u", GetLastError());
                }
                break;
            }

            case GD_SESS_DETACH:
            {
                if(!DebugActiveProcessStop(pstTargetInfo->dwPID))
                {
                    MessageBox(pstTargetInfo->pstDebugInfoFromGui->hMainWindow, L"Cannot detach from target. See log file.", L"Error", MB_ICONERROR);
                    logerror(pstLogger, L"%s(): DebugActiveProcessStop() failed %u", GetLastError());
                }
                else
                {
                    goto gui_exit;
                }
                break;
            }

            case GD_SESS_DUMPTERM:
            {
                break;
            }

            default:
            {
                ASSERT(FALSE);
                break;
            }
        }
    }
    
    return TRUE;

gui_exit:
    return FALSE;
}

static BOOL fProcessDebugEventLoop(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);

    DEBUG_EVENT de;

    BOOL fRetVal;

    //logtrace("Debug thread: Waiting for debug event...");

    fRetVal = TRUE;
    if(WaitForDebugEvent(&de, 10))
    {
        if(de.dwProcessId != pstTargetInfo->dwPID)
        {
            logtrace(pstLogger, L"Debug thread: Received debug event for sibling process %u != %u", de.dwProcessId, pstTargetInfo->dwPID);
            dbgwprintf(L"Debug thread: Received debug event for sibling process %u != %u", de.dwProcessId, pstTargetInfo->dwPID);
        }
        else
        {
            pstTargetInfo->lpDebugEvent = &de;

            switch(de.dwDebugEventCode)
            {
                case EXCEPTION_DEBUG_EVENT:
                {
                    DWORD dwContinueStatus;

                    fOnException(pstTargetInfo, &dwContinueStatus);
                    ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);
                
                    return TRUE;
                }

                case CREATE_THREAD_DEBUG_EVENT:
                {
                    fOnCreateThread(pstTargetInfo);
                    break;
                }

                case CREATE_PROCESS_DEBUG_EVENT:
                {
                    fOnCreateProcess(pstTargetInfo);
                    break;
                }

                case EXIT_THREAD_DEBUG_EVENT:
                {
                    fOnExitThread(pstTargetInfo);
                    break;
                }

                case EXIT_PROCESS_DEBUG_EVENT:
                {
                    fRetVal = fOnExitProcess(pstTargetInfo);
                    break;
                }

                case LOAD_DLL_DEBUG_EVENT:
                {
                    fOnLoadDll(pstTargetInfo);
                    break;
                }

                case UNLOAD_DLL_DEBUG_EVENT:
                {
                    fOnUnloadDll(pstTargetInfo);
                    break;
                }

                case OUTPUT_DEBUG_STRING_EVENT:
                {
                    fOnOutputDebugString(pstTargetInfo);
                    break;
                }

                case RIP_EVENT:
                {
                    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
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
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);

    }// if-else

    return fRetVal;
}

BOOL fOnException(PTARGETINFO pstTargetInfo, __out DWORD *pdwContinueStatus)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    DWORD dwExCode = lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode;
    WCHAR wsExString[SLEN_EXCEPTION_NAME];
    WCHAR wsExceptionMessage[SLEN_COMMON64];

    wprintf(
        L"Exception 0x%08X at 0x%08x\n", 
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

        MessageBox(NULL, wsExceptionMessage, L"Exception Raised", MB_ICONSTOP|MB_OK);
    }
    
    IFPTR_SETVAL(pdwContinueStatus, DBG_EXCEPTION_NOT_HANDLED);
    return TRUE;
}

BOOL fOnCreateThread(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    dbgwprintf(L"Thread create: [%u]\n", lpDebugEvent->dwThreadId);
    if(!fAddThread(pstTargetInfo->phtThreads, lpDebugEvent->dwThreadId, &(lpDebugEvent->u.CreateThread)))
    {
        logerror(pstLogger, L"Unable to insert thread to hashtable");
    }

    ++(pstTargetInfo->nTotalThreads);
    ++(pstTargetInfo->nCurThreads);

    return TRUE;
}

// Handles the CREATE_PROCESS_DEBUG_EVENT.
// Increments the thread and process counters,
// Copies process info into the target info structure.
// Return: TRUE always
BOOL fOnCreateProcess(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    CREATE_THREAD_DEBUG_INFO stThreadInfo;

#ifdef _DEBUG
    WCHAR wsImageName[SLEN_MAXPATH];
    DWORD dwRet = GetFinalPathNameByHandle(lpDebugEvent->u.CreateProcessInfo.hFile, wsImageName, SLEN_MAXPATH - 1, VOLUME_NAME_NT);
    wsImageName[SLEN_MAXPATH - 1] = 0;

    if(dwRet == 0)
        logerror(pstLogger, L"Could not get filename from handle");
    else if(dwRet > SLEN_MAXPATH)
        logerror(pstLogger, L"Insufficient buffer size for GetFinalPathNameByHandle()");
    else
    {
        wprintf(L"Process create: [%u] : %s\nBaseAddr: 0x%08x\nStartAddr: 0x%08x\nThreadLocal: 0x%08x\n", 
            lpDebugEvent->dwProcessId, 
            wsImageName, 
            lpDebugEvent->u.CreateProcessInfo.lpBaseOfImage,
            lpDebugEvent->u.CreateProcessInfo.lpStartAddress,
            lpDebugEvent->u.CreateProcessInfo.lpThreadLocalBase);
    }
#endif

    // Save main thread info now because we do not receive a CREATE_THREAD_DEBUG_EVENT
    // for the main thread.
    if(!pstTargetInfo->fCreateProcessEventRecvd)
    {
        pstTargetInfo->fCreateProcessEventRecvd = TRUE;

        stThreadInfo.hThread = lpDebugEvent->u.CreateProcessInfo.hThread;
        stThreadInfo.lpStartAddress = lpDebugEvent->u.CreateProcessInfo.lpStartAddress;
        stThreadInfo.lpThreadLocalBase = lpDebugEvent->u.CreateProcessInfo.lpThreadLocalBase;

        if(!fAddThread(pstTargetInfo->phtThreads, lpDebugEvent->dwThreadId, &stThreadInfo))
        {
            // todo: 
        }

        // Save all info from CREATE_PROCESS_DEBUG_INFO for later use
        memcpy(&(pstTargetInfo->stProcessInfo), &(lpDebugEvent->u.CreateProcessInfo), sizeof(CREATE_PROCESS_DEBUG_INFO));
    }

    ++(pstTargetInfo->nCurThreads);
    ++(pstTargetInfo->nTotalThreads);
    ++(pstTargetInfo->nTotalProcesses);

    // Close handle to the image file, we do not need here... yet
    CloseHandle(lpDebugEvent->u.CreateProcessInfo.hFile);

    return TRUE;
}

BOOL fOnExitThread(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    dbgwprintf(L"Thread exit: [%u] : ExitCode %d\n", lpDebugEvent->dwThreadId, lpDebugEvent->u.ExitThread.dwExitCode);
    if(!fRemoveThread(pstTargetInfo->phtThreads, lpDebugEvent->dwThreadId, NULL))
    {
        logerror(pstLogger, L"Failed to remove thread from hashtable");
    }

    --(pstTargetInfo->nCurThreads);

    return TRUE;
}

BOOL fOnExitProcess(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    BOOL fRetVal = TRUE;
    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    dbgwprintf(L"Process exit: [%u] : ExitCode %d\r\n", lpDebugEvent->dwProcessId, lpDebugEvent->u.ExitProcess.dwExitCode);
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

BOOL fOnLoadDll(PTARGETINFO pstTargetInfo)
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
    else if(dwRet > SLEN_MAXPATH)
        logerror(pstLogger, L"Insufficient buffer size for GetFinalPathNameByHandle()");
    else
    {
        wprintf(L"DLL Load: [0x%08x] : %s\n", lpDebugEvent->u.LoadDll.lpBaseOfDll, wsImageName);
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

BOOL fOnUnloadDll(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstTargetInfo->lpDebugEvent);

    LPDEBUG_EVENT lpDebugEvent = pstTargetInfo->lpDebugEvent;

    WCHAR *pws = NULL;
    int outValSize = 0;

    if(!fChlDsFindHT(pstTargetInfo->phtDllsLoaded, &(lpDebugEvent->u.UnloadDll.lpBaseOfDll), sizeof(DWORD), &pws, &outValSize))
    {
        wprintf(L"DLL Unload : [0x%08x] : Image name not found\n", (DWORD)(lpDebugEvent->u.UnloadDll.lpBaseOfDll));
    }
    else
    {
        wprintf(L"DLL Unload : [0x%08x] : %s\n", (DWORD)lpDebugEvent->u.UnloadDll.lpBaseOfDll, pws);
        fChlDsRemoveHT(pstTargetInfo->phtDllsLoaded, &(lpDebugEvent->u.UnloadDll.lpBaseOfDll), sizeof(DWORD));
    }

    --(pstTargetInfo->nCurDllsLoaded);

    return TRUE;
}

BOOL fOnOutputDebugString(PTARGETINFO pstTargetInfo)
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

            wprintf(L"OutputDebugString: [%u][%s]\n", bytesRead, pwsDebugString);
        }
        else
        {
            ReadProcessMemory(
                pstTargetInfo->stProcessInfo.hProcess, 
                lpDebugEvent->u.DebugString.lpDebugStringData, 
                pwsDebugString,
                lpDebugEvent->u.DebugString.nDebugStringLength * sizeof(char), 
                &bytesRead);

            wprintf(L"OutputDebugString: [%u][%S]\n", bytesRead, pwsDebugString);
        }
        vChlMmFree((void**)&pwsDebugString);
    }

    return TRUE;
}

static void vSetMenuItemsState(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);

    switch(pstTargetInfo->iDebugState)
    {
        case STATE_DBG_RUNNING:
        {
            vMiDebuggerRunning(pstTargetInfo->pstDebugInfoFromGui->hMainMenu);
            break;
        }

        case STATE_DBG_DEBUGGING:
        {
            vMiDebuggerDebugging(pstTargetInfo->pstDebugInfoFromGui->hMainMenu);
            break;
        }

        default:
        {
            ASSERT(FALSE);
            break;
        }
    }
}
