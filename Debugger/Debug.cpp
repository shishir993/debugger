
#include "Inc\Debug.h"

extern PLOGGER pstLogger;
extern HINSTANCE g_hMainInstance;

typedef struct _TargetInfo {
    PDEBUGINFO pstDebugInfoFromGui;
    DWORD dwPID;
    CHL_HTABLE phtDllsLoaded;
    CHL_HTABLE phtThreads;
}TARGETINFO, *PTARGETINFO;

static BOOL fDebugNewProgram(PTARGETINFO pstTargetInfo);
static BOOL fDebugActiveProcess(PTARGETINFO pstTargetInfo);
static void vOnThisThreadExit(PTARGETINFO *ppstTargetInfo);
// static BOOL fDebugEventLoop();

DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs)
{
    ASSERT(lpvArgs);

    DWORD dwErrCode = ERROR_SUCCESS;

    PTARGETINFO pstTargetInfo = NULL;

    HANDLE hInitSyncEvent = NULL;

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
        fDebugActiveProcess(pstTargetInfo);
    }
    else
    {
        fDebugNewProgram(pstTargetInfo);
    }

    // Signal init sync event to Gui thread
    ASSERT(pstTargetInfo->pstDebugInfoFromGui->pszInitSyncEvtName);
    if( (hInitSyncEvent = OpenEvent(SYNCHRONIZE, FALSE, pstTargetInfo->pstDebugInfoFromGui->pszInitSyncEvtName)) == NULL )
    {
        SET_ERRORCODE(dwErrCode);
        vWriteLog(pstLogger, L"%s(): OpenEvent() failed %u", __FUNCTIONW__, dwErrCode);
        goto error_return;
    }
    SetEvent(hInitSyncEvent);
    FREE_HANDLE(hInitSyncEvent);
    pstTargetInfo->pstDebugInfoFromGui->pszInitSyncEvtName = NULL;

    // Start from-gui message loop and debug event loop
    // 

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
    // Set up required data structures before starting debugging loop
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
        vChlMmFree((void**)&plocal->pstDebugInfoFromGui->pszInitSyncEvtName);
    }
    else
    {
        ASSERT(plocal->pstDebugInfoFromGui->pszTargetPath == NULL);
    }

    vChlMmFree((void**)&plocal);
    *ppstTargetInfo = NULL;
    return;
}
