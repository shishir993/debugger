
#include "Inc\DebugHelpers.h"
#include "Inc\Breakpoint.h"
#include "Inc\GuiManager.h"

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

BOOL fBreakAtEntryPoint(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo && pstTargetInfo->pListBreakpoint);

    BPINFO stBpInfo;

    stBpInfo.iBpType = BPTYPE_USERSINGLEHIT | BPTYPE_ASMLEVEL;
    stBpInfo.dwTargetAddr = (DWORD)pstTargetInfo->stProcessInfo.lpStartAddress;

    // TODO: take the return breakpoint ID value
    return fBpInsert(pstTargetInfo->pListBreakpoint, &stBpInfo, pstTargetInfo, NULL);
}

BOOL fHandleExceptionBreakpoint(PTARGETINFO pstTargetInfo, __out PDWORD pdwContinueStatus)
{
    ASSERT(pstTargetInfo && pstTargetInfo->lpDebugEvent);
    ASSERT(pdwContinueStatus);

    BPINFO stBpInfo;

    ZeroMemory(&stBpInfo, sizeof(BPINFO));

    // Is it a USER or single-hit?
    stBpInfo.dwTargetAddr = (DWORD)pstTargetInfo->lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    stBpInfo.iBpType = BPTYPE_ASMLEVEL;
    if(!fBpFind(pstTargetInfo->pListBreakpoint, &stBpInfo, 0))
    {
        logwarn(pstLogger, L"%s(): fBpFind() failed %u", __FUNCTIONW__, GetLastError());
        
        // Ask user what to do and get the pdwContinueStatus parameter set
        vSetContinueStatusFromUser(
            pstTargetInfo->lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode,
            (DWORD)pstTargetInfo->lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress,
            pstTargetInfo->lpDebugEvent->u.Exception.dwFirstChance,
            pdwContinueStatus);

        return TRUE;
    }

    // Found the breakpoint info
    if(stBpInfo.iBpType & BPTYPE_USERSINGLEHIT || stBpInfo.iBpType & BPTYPE_USERMULTIHIT)
    {
        // Save this breakpoint information so that we resume target execution(by removing BP) later.
        pstTargetInfo->stPrevBpInfo.dwThreadId = pstTargetInfo->lpDebugEvent->dwThreadId;
        memcpy(&pstTargetInfo->stPrevBpInfo.stBpInfo, &stBpInfo, sizeof(BPINFO));

        pstTargetInfo->iPrevDebugState = pstTargetInfo->iDebugState;
        pstTargetInfo->iDebugState = DSTATE_BREAKPOINTWAIT;
    }
    else if(stBpInfo.iBpType & BPTYPE_DEBUGGERSINGLEHIT || stBpInfo.iBpType & BPTYPE_DEBUGGERMULTIHIT)
    {
        // TODO:
    }
    else
    {
        // should never happen
        ASSERT(FALSE);
    }

    *pdwContinueStatus = DBG_CONTINUE;

    return TRUE;
}

void vSetContinueStatusFromUser(DWORD dwExceptionCode, DWORD dwExceptionAddress, BOOL fFirstChance, PDWORD pdwContinueStatus)
{
    ASSERT(dwExceptionCode > 0 && dwExceptionAddress > 0);
    ASSERT(pdwContinueStatus);

    WCHAR szExString[SLEN_EXCEPTION_NAME];
    WCHAR szExceptionMessage[SLEN_COMMON128];

    int iUserChoice;

    // Construct the string to show to user
    if(!fGetExceptionName(dwExceptionCode, szExString, _countof(szExString)))
    {
        logwarn(pstLogger, L"Could not get exception name for exception code %x", dwExceptionCode);

        swprintf_s(szExceptionMessage, 
                _countof(szExceptionMessage), 
                L"Unknown Exception(0x%08x) at: 0x%08x\r\nPress Retry to Break and Debug", 
                dwExceptionCode, 
                dwExceptionAddress);

    }
    else
    {
        if(fFirstChance)
        {
            swprintf_s(
                szExceptionMessage, 
                _countof(szExceptionMessage), 
                L"%s Exception(0x%08x)(FirstChance) at: 0x%08x\r\n\nPress Retry to Break and Debug the Application", 
                szExString, 
                dwExceptionCode,
                dwExceptionAddress);
        }
        else
        {
            swprintf_s(
                szExceptionMessage, 
                _countof(szExceptionMessage), 
                L"%s Exception(0x%08x) at: 0x%08x\r\n\nPress Retry to Break and Debug the Application", 
                szExString,
                dwExceptionCode, 
                dwExceptionAddress);
        }
    }

    iUserChoice = MessageBox(NULL, szExceptionMessage, L"Exception hit in target", MB_ABORTRETRYIGNORE|MB_ICONERROR);
    switch(iUserChoice)
    {
        case IDABORT:
        {   
            // User wants to abort target process immediately
            *pdwContinueStatus = DBG_CONTCUSTOM_ABORT;
            break;
        }

        case IDRETRY:
        {
            // User wants to break and enter debugging mode
            *pdwContinueStatus = DBG_CONTCUSTOM_BREAK;
            break;
        }

        case IDIGNORE:
        {
            *pdwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
            break;
        }

        default:
        {
            // Must never happen
            ASSERT(FALSE);
            break;
        }
    }// switch(userChoice)

    return;
}

BOOL fDecrementInstPointer(CHL_HTABLE *phtThreads, DWORD dwThreadId)
{
    ASSERT(phtThreads);
    ASSERT(dwThreadId > 0);

    HANDLE hThread;
    CONTEXT stThreadContext;

    if(!fGetThreadHandle(phtThreads, dwThreadId, &hThread))
    {
        logerror(pstLogger, L"%s(): Cannot get thread handle for thread %u", __FUNCTIONW__, dwThreadId);
        goto error_return;
    }

    ZeroMemory(&stThreadContext, sizeof(CONTEXT));

    stThreadContext.ContextFlags = CONTEXT_CONTROL;
    if(!GetThreadContext(hThread, &stThreadContext))
    {
        logerror(pstLogger, L"%s(): GetThreadContext failed %u", __FUNCTIONW__, GetLastError());
        goto error_return;
    }

    dbgwprintf(L"EIP before decrementing: 0x%08x\n", stThreadContext.Eip);
    --stThreadContext.Eip;

    dbgwprintf(L"EIP after decrementing: 0x%08x\n", stThreadContext.Eip);
    stThreadContext.ContextFlags = CONTEXT_CONTROL;
    if(!SetThreadContext(hThread, &stThreadContext))
    {
        logerror(pstLogger, L"%s(): SetThreadContext failed %u", __FUNCTIONW__, GetLastError());
        goto error_return;
    }

    return TRUE;

error_return:
    return FALSE;
}

BOOL fUpdateThreadsListView(HWND hList, CHL_HTABLE *phtThreads, HANDLE hMainThread)
{
    ASSERT(ISVALID_HANDLE(hList));
    ASSERT(phtThreads);

    BOOL fRetVal = TRUE;

    CHL_HT_ITERATOR itr;
    int keysize, valsize;

    DWORD dwThreadID = 0;
    LPCREATE_THREAD_DEBUG_INFO pThreadDbgInfo = NULL;

    int nThreads;
    PLV_THREADINFO pstThreadInfo = NULL;

    DWORD dwEip;
    int iThreadPri;

    CONTEXT stContext;

    // Create memory to hold thread info for display
    // Assume 32 max threads now because there is no easy way to get this info
    // from the hashtable
    // TODO: Handle dynamic number of threads
    if(!fChlMmAlloc((void**)&pstThreadInfo, sizeof(LV_THREADINFO) * 32, NULL))
    {
        return FALSE;
    }

    ZeroMemory(&stContext, sizeof(stContext));

    nThreads = 0;
    fChlDsInitIteratorHT(&itr);
    while(fChlDsGetNextHT(phtThreads, &itr, &dwThreadID, &keysize, &pThreadDbgInfo, &valsize))
    {
        ASSERT(pThreadDbgInfo);
        ASSERT(ISVALID_HANDLE(pThreadDbgInfo->hThread));

        pstThreadInfo[nThreads].dwThreadId = dwThreadID;

        // Get EIP value
        stContext.ContextFlags = CONTEXT_CONTROL;
        if(!GetThreadContext(pThreadDbgInfo->hThread, &stContext))
        {
            dbgwprintf(L"%s(): GetThreadPriority() failed for id = %u, handle = 0x%08x", __FUNCTIONW__, dwThreadID, pThreadDbgInfo->hThread);
            logwarn(pstLogger, L"%s(): GetThreadContext() failed for id = %u, handle = 0x%08x", __FUNCTIONW__, dwThreadID, pThreadDbgInfo->hThread);
            pstThreadInfo[nThreads].dwEIPLocation = 0;
        }
        else
        {
            pstThreadInfo[nThreads].dwEIPLocation = stContext.Eip;
        }

        // Determine main / worker thread
        pstThreadInfo[nThreads].thType = pThreadDbgInfo->hThread == hMainThread ? THTYPE_MAIN : THTYPE_WORKER;

        // Get priority
        pstThreadInfo[nThreads].iThreadPri = GetThreadPriority(pThreadDbgInfo->hThread);
        if(pstThreadInfo[nThreads].iThreadPri == THREAD_PRIORITY_ERROR_RETURN)
        {
            dbgwprintf(L"%s(): GetThreadPriority() failed for id = %u, handle = 0x%08x", __FUNCTIONW__, dwThreadID, pThreadDbgInfo->hThread);
            logwarn(pstLogger, L"%s(): GetThreadPriority() failed for id = %u, handle = 0x%08x", __FUNCTIONW__, dwThreadID, pThreadDbgInfo->hThread);
        }

        pstThreadInfo[nThreads].szFunction[0] = 0;

        ++nThreads;
    }

    // Update listview
    fRetVal = fGuiUpdateThreadsList(hList, pstThreadInfo, nThreads);

    vChlMmFree((void**)&pstThreadInfo);

    return fRetVal;
}
