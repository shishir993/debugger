
#include "Inc\DebugHelpers.h"

extern PLOGGER pstLogger;

BOOL fGetExceptionName(DWORD excode, __out WCHAR *pwsBuffer, int bufSize)
{
    int iStringID = 0;

    switch(excode)
    {
        case EXCEPTION_ACCESS_VIOLATION: iStringID = IDS_EX_ACCESSVIOL; break;

        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: iStringID = IDS_EX_ARRBOUNDS; break;

        case EXCEPTION_BREAKPOINT: iStringID = IDS_EX_BREAKPNT; break;

        case EXCEPTION_DATATYPE_MISALIGNMENT: iStringID = IDS_EX_DATAMISALIGN; break;

        case EXCEPTION_FLT_DENORMAL_OPERAND: iStringID = IDS_EX_FLT_DENOP; break;

        case EXCEPTION_FLT_DIVIDE_BY_ZERO: iStringID = IDS_EX_FLT_DIVZERO; break;

        case EXCEPTION_FLT_INEXACT_RESULT: iStringID = IDS_EX_FLT_INEXACT; break;

        case EXCEPTION_FLT_INVALID_OPERATION: iStringID = IDS_EX_FLT_INVOP; break;

        case EXCEPTION_FLT_OVERFLOW: iStringID = IDS_EX_FLT_OVRFLOW; break;

        case EXCEPTION_FLT_STACK_CHECK: iStringID = IDS_EX_FLT_STKFLOW; break;

        case EXCEPTION_FLT_UNDERFLOW: iStringID = IDS_EX_FLT_UNDFLOW; break;

        case EXCEPTION_ILLEGAL_INSTRUCTION: iStringID = IDS_EX_INVINST; break;

        case EXCEPTION_IN_PAGE_ERROR: iStringID = IDS_EX_INPAGEERR; break;

        case EXCEPTION_INT_DIVIDE_BY_ZERO: iStringID = IDS_EX_INT_DIVZERO; break;

        case EXCEPTION_INT_OVERFLOW: iStringID = IDS_EX_INT_OVRFLOW; break;

        case EXCEPTION_INVALID_DISPOSITION: iStringID = IDS_EX_INVDISP; break;

        case EXCEPTION_NONCONTINUABLE_EXCEPTION: iStringID = IDS_EX_NONCONT; break;

        case EXCEPTION_PRIV_INSTRUCTION: iStringID = IDS_EX_PRIVINST; break;

        case EXCEPTION_SINGLE_STEP: iStringID = IDS_EX_SINGLESTEP; break;

        case EXCEPTION_STACK_OVERFLOW: iStringID = IDS_EX_STKOVRLFLOW; break;

        default:
        {
            swprintf_s(pwsBuffer, bufSize, L"Unknown Exception");
            return FALSE;
        }
    }

    return (LoadString(GetModuleHandle(NULL), iStringID, pwsBuffer, bufSize) != 0);
}

BOOL fAddThread(CHL_HTABLE *phtThreads, DWORD dwThreadId, LPCREATE_THREAD_DEBUG_INFO lpThreadInfo)
{
    ASSERT(phtThreads);
    ASSERT(lpThreadInfo);

    LPCREATE_THREAD_DEBUG_INFO pThreadDbgInfo = NULL;

    if(!fChlMmAlloc((void**)&pThreadDbgInfo, sizeof(CREATE_THREAD_DEBUG_INFO), NULL))
        return FALSE;

    memcpy(pThreadDbgInfo, lpThreadInfo, sizeof(CREATE_THREAD_DEBUG_INFO));
    return fChlDsInsertHT(phtThreads, &dwThreadId, sizeof(DWORD), pThreadDbgInfo, sizeof(LPCREATE_THREAD_DEBUG_INFO));
}

BOOL fRemoveThread(CHL_HTABLE *phtThreads, DWORD dwThreadId, __out LPCREATE_THREAD_DEBUG_INFO lpThreadInfo)
{
    ASSERT(phtThreads);

    DBG_UNREFERENCED_PARAMETER(lpThreadInfo);

    return fChlDsRemoveHT(phtThreads, &dwThreadId, sizeof(DWORD));
}

BOOL fGetThreadHandle(CHL_HTABLE *phtThreads, DWORD dwThreadId, __out HANDLE *phThread)
{
    ASSERT(phtThreads);
    ASSERT(phThread);

    LPCREATE_THREAD_DEBUG_INFO pThreadDbgInfo = NULL;
    int outValSize = 0;

    if(!fChlDsFindHT(phtThreads, &dwThreadId, sizeof(DWORD), &pThreadDbgInfo, &outValSize))
        return FALSE;

    ASSERT(outValSize == sizeof(LPCREATE_THREAD_DEBUG_INFO));
    ASSERT(pThreadDbgInfo);

    *phThread = pThreadDbgInfo->hThread;
    return TRUE;
}

BOOL fDeleteThreadsTable(CHL_HTABLE *phtThreads)
{
    ASSERT(phtThreads);

    CHL_HT_ITERATOR itr;

    DWORD dwThreadID = 0;
    LPCREATE_THREAD_DEBUG_INFO *ppThreadDbgInfo = NULL;

    int keysize, valsize;

    fChlDsInitIteratorHT(&itr);
    while(fChlDsGetNextHT(phtThreads, &itr, &dwThreadID, &keysize, &ppThreadDbgInfo, &valsize))
    {
        ASSERT(ppThreadDbgInfo && *ppThreadDbgInfo);
        vChlMmFree((void**)ppThreadDbgInfo);
    }
    
    return fChlDsDestroyHT(phtThreads);
}

BOOL fSuspendAllThreads(CHL_HTABLE *phtThreads)
{
    ASSERT(phtThreads);

    CHL_HT_ITERATOR itr;

    DWORD dwThreadID = 0;
    LPCREATE_THREAD_DEBUG_INFO pThreadDbgInfo = NULL;

    int keysize, valsize;
    BOOL fRetVal = TRUE;

    fChlDsInitIteratorHT(&itr);
    while(fChlDsGetNextHT(phtThreads, &itr, &dwThreadID, &keysize, &pThreadDbgInfo, &valsize))
    {
        ASSERT(pThreadDbgInfo);
        ASSERT(ISVALID_HANDLE(pThreadDbgInfo->hThread));

        dbgwprintf(L"Suspending thread %u\n", dwThreadID);

        if( SuspendThread(pThreadDbgInfo->hThread) == -1 )
        {
            logerror(pstLogger, L"SuspendThread() failed for thread %u: %u", dwThreadID, GetLastError());
            fRetVal = FALSE;
        }
    }

    return fRetVal;
}


BOOL fResumeAllThreads(CHL_HTABLE *phtThreads)
{
    ASSERT(phtThreads);

    CHL_HT_ITERATOR itr;

    DWORD dwThreadID = 0;
    LPCREATE_THREAD_DEBUG_INFO pThreadDbgInfo = NULL;

    int keysize, valsize;
    BOOL fRetVal = TRUE;

    fChlDsInitIteratorHT(&itr);
    while(fChlDsGetNextHT(phtThreads, &itr, &dwThreadID, &keysize, &pThreadDbgInfo, &valsize))
    {
        ASSERT(pThreadDbgInfo);
        ASSERT(ISVALID_HANDLE(pThreadDbgInfo->hThread));

        dbgwprintf(L"Resuming thread %u\n", dwThreadID);

        if( ResumeThread(pThreadDbgInfo->hThread) == -1 )
        {
            logerror(pstLogger, L"ResumeThread() failed for thread %u: %u", dwThreadID, GetLastError());
            fRetVal = FALSE;
        }
    }

    return fRetVal;
}

BOOL fIsNtDllLoaded(CHL_HTABLE *phtDllTable, __out DWORD *pdwBaseAddress)
{
    ASSERT(phtDllTable);

    CHL_HT_ITERATOR itr;

    DWORD dwBase = 0;
    WCHAR *psDllName = NULL;
    WCHAR *psDllFilename = NULL;

    int keysize, valsize;

    fChlDsInitIteratorHT(&itr);
    while(fChlDsGetNextHT(phtDllTable, &itr, &dwBase, &keysize, &psDllName, &valsize))
    {
        ASSERT(psDllName && valsize > 0);
        if((psDllFilename = pszChlSzGetFilenameFromPath(psDllName, valsize)) == NULL)
        {
            logerror(pstLogger, L"fIsNtDllLoaded(): Error reading filename from path: %s\n", psDllName);
            continue;
        }
        else if(wcscmp(L"ntdll.dll", psDllFilename) == 0)
        {
            IFPTR_SETVAL(pdwBaseAddress, dwBase);
            return TRUE;
        }
    }

    IFPTR_SETVAL(pdwBaseAddress, 0);
    return FALSE;
}
