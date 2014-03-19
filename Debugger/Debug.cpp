
#include "Inc\Debug.h"

extern PLOGGER pstLogger;
extern HINSTANCE g_hMainInstance;

typedef struct _TargetInfo {
    PDEBUGINFO pstDebugInfoFromGui;
    DWORD dwPID;
    DWORD dwMainThreadID;
    CHL_HTABLE *phtDllsLoaded;
    CHL_HTABLE *phtThreads;
}TARGETINFO, *PTARGETINFO;

static BOOL fDebugNewProgram(PTARGETINFO pstTargetInfo);
static BOOL fDebugActiveProcess(PTARGETINFO pstTargetInfo);
static void vOnThisThreadExit(PTARGETINFO *ppstTargetInfo);
static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);
static BOOL fProcessDebugEventLoop(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode);

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
                        DEBUG_ONLY_THIS_PROCESS|
                        CREATE_NEW_CONSOLE,    // creation flags
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
    // free Dlls table

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

    vChlMmFree((void**)&plocal);
    *ppstTargetInfo = NULL;
    return;
}

static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);

    error_return:
    if(pdwErrCode) { *pdwErrCode = ERROR_CALL_NOT_IMPLEMENTED; }
    return FALSE;
}

static BOOL fProcessDebugEventLoop(PTARGETINFO pstTargetInfo, __out DWORD *pdwErrCode)
{
    ASSERT(pstTargetInfo);

    error_return:
    if(pdwErrCode) { *pdwErrCode = ERROR_CALL_NOT_IMPLEMENTED; }
    return FALSE;
}
