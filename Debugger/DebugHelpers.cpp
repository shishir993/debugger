
#include "Inc\DebugHelpers.h"
#include "Inc\MenuItems.h"
#include "Inc\Breakpoint.h"
#include "Inc\GuiManager.h"
#include "Dasm\DasmEngine.h"

extern PLOGGER pstLogger;

void vPrintBits(DWORD dwNumber)
{
    for(int index = sizeof(dwNumber) * 8; index > 0; --index)
    {
        if(index % 4 == 0)
        {
            wprintf(L" ");
        }
        wprintf(L"%d", dwNumber & (1 << (index - 1)) ? 1 : 0);
    }
}

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
        
        if(pstTargetInfo->iDebugState == DSTATE_WAITFOR_DBGBREAK)
        {
            // We have received the breakpoint exception after calling DebugBreakProcess

            // Save the thread id and target address so that we can show disassembly
            pstTargetInfo->stPrevBpInfo.dwThreadId = pstTargetInfo->lpDebugEvent->dwThreadId;
            pstTargetInfo->stPrevBpInfo.stBpInfo.dwTargetAddr = (DWORD)pstTargetInfo->lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress;

            // Debugger state change is handled in Debug.cpp:fOnException()

            *pdwContinueStatus = DBG_CONTCUSTOM_BREAK;

            return TRUE;
        }

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
        // Remove the breakpoint here so that we can show the disassembly with the correct opcode
        if(!fBpRemove(pstTargetInfo->pListBreakpoint, &stBpInfo, pstTargetInfo))
        {
            logerror(pstLogger, L"%s(): fBpRemove() failed %u", __FUNCTIONW__, GetLastError());
            return FALSE;
        }

        // Decrement instruction pointer for the same reason
        fDecrementInstPointer(pstTargetInfo->phtThreads, pstTargetInfo->lpDebugEvent->dwThreadId);

        // Save this breakpoint information so that we can re-insert breakpoint 
        // if needed when continuing execution later on.
        pstTargetInfo->stPrevBpInfo.dwThreadId = pstTargetInfo->lpDebugEvent->dwThreadId;
        memcpy(&pstTargetInfo->stPrevBpInfo.stBpInfo, &stBpInfo, sizeof(BPINFO));

        vDebuggerStateChange(pstTargetInfo, DSTATE_BREAKPOINTWAIT);
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

BOOL fReInsertBPIf(PTARGETINFO pstTargetInfo, PREVBPINFO *pstBpInfo)
{
    ASSERT(pstTargetInfo);
    ASSERT(pstBpInfo);

    ASSERT(pstBpInfo->dwThreadId != 0);

    if(pstTargetInfo->iPrevDebugState == DSTATE_WAITFOR_DBGBREAK)
    {
        logtrace(pstLogger, L"%s(): Not reinserting because prev state was DSTATE_WAITFOR_DBGBREAK", __FUNCTIONW__);
        return TRUE;
    }

    if(pstBpInfo->stBpInfo.iBpType & BPTYPE_USERMULTIHIT || pstBpInfo->stBpInfo.iBpType & BPTYPE_DEBUGGERMULTIHIT)
    {
        logtrace(pstLogger, L"%s(): Reinserting multi-hit BP", __FUNCTIONW__);
        return fBpRemove(pstTargetInfo->pListBreakpoint, &pstBpInfo->stBpInfo, pstTargetInfo);
    }

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

BOOL fSetTrapFlag(CHL_HTABLE *phtThreads, DWORD dwThreadId)
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

    dbgwprintf(L"Setting TF...\n");

    vPrintBits(stThreadContext.EFlags);
    dbgwprintf(L" : EFLAGS before\n");

    stThreadContext.EFlags |= (1 << BITPOS_EFLAGS_TF);

    vPrintBits(stThreadContext.EFlags);
    dbgwprintf(L" : EFLAGS after\n");

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

BOOL fClearTrapFlag(CHL_HTABLE *phtThreads, DWORD dwThreadId)
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

    dbgwprintf(L"Clearing TF...\n");

    vPrintBits(stThreadContext.EFlags);
    dbgwprintf(L" : EFLAGS before\n");

    stThreadContext.EFlags &= ~(1 << BITPOS_EFLAGS_TF);

    vPrintBits(stThreadContext.EFlags);
    dbgwprintf(L" : EFLAGS after\n");

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

            fRetVal = FALSE;
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

            fRetVal = FALSE;
        }

        pstThreadInfo[nThreads].szFunction[0] = 0;

        ++nThreads;
    }

    // Update listview
    fRetVal &= fGuiUpdateThreadsList(hList, pstThreadInfo, nThreads);

    vChlMmFree((void**)&pstThreadInfo);

    return fRetVal;
}

BOOL fUpdateRegistersListView(HWND hList, DWORD dwThreadId)
{
    ASSERT(ISVALID_HANDLE(hList));
    ASSERT(dwThreadId > 0);

    CONTEXT stContext;
    HANDLE hThread = NULL;

    DWORD adwValues[_countof(apszRegNames)];

    ASSERT(_countof(apszRegNames) == 9);

    hThread = OpenThread(THREAD_GET_CONTEXT, FALSE, dwThreadId);
    if(hThread == NULL)
    {
        logerror(pstLogger, L"%s(): OpenThread() failed %u", __FUNCTIONW__, GetLastError());
        return FALSE;
    }

    stContext.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
    if(!GetThreadContext(hThread, &stContext))
    {
        logerror(pstLogger, L"%s(): GetThreadContext() failed %u", __FUNCTIONW__, GetLastError());
        CloseHandle(hThread);
        return FALSE;
    }

    CloseHandle(hThread);

    adwValues[0] = stContext.Eax;
    adwValues[1] = stContext.Ebx;
    adwValues[2] = stContext.Ecx;
    adwValues[3] = stContext.Edx;
    adwValues[4] = stContext.Esi;
    adwValues[5] = stContext.Edi;
    adwValues[6] = stContext.Esp;
    adwValues[7] = stContext.Ebp;
    adwValues[8] = stContext.Eip;

    return fGuiUpdateRegistersList(hList, apszRegNames, adwValues, _countof(apszRegNames));
}

// This function must be called whenever the state of the debugger must be changed,
// i.e., of pstTargetInfo->iDebugState must be changed. This function is required
// because on state change, there are certain UI updates that is to be undertaken.
// 
void vDebuggerStateChange(PTARGETINFO pstTargetInfo, int iNewState)
{
    ASSERT(pstTargetInfo);
    ASSERT(iNewState >= DSTATE_START && iNewState <= DSTATE_END);

    /*
     * For now, handle these state changes:
     * running -> debugging
     * debugging -> running
     */

    int iOlderState = pstTargetInfo->iPrevDebugState;
    int iCurState = pstTargetInfo->iDebugState;

    ASSERT(iCurState != iNewState);

    DBG_UNREFERENCED_LOCAL_VARIABLE(iOlderState);

    switch(iNewState)
    {
        case DSTATE_RUNNING:
        case DSTATE_SINGLESTEP_AFTER:
        {
            // Clear/Gray-out all child controls in the tab page
            // because that information is not valid when target is running

            break;
        }
        
        case DSTATE_DEBUGGING:
        //case DSTATE_BREAKPOINTWAIT:   // same as DSTATE_DEBUGGING
        {
            DWORD dwThreadIdToUse;
            DWORD dwTargetAddrToUse;

            // Update UI to have updated target information displayed
            
            // 1. Update threads information
            if(!fUpdateThreadsListView(
                    pstTargetInfo->pstDebugInfoFromGui->stTabPageInfo.hListThreads, 
                    pstTargetInfo->phtThreads,
                    pstTargetInfo->stProcessInfo.hThread))
            {
                logerror(pstLogger, L"%s(): fUpdateThreadsListView failed %u", __FUNCTIONW__, GetLastError());

                // TODO: bubble up the error or show MessageBox?
            }

            // 2. Update CPU register values
            // TODO: handle error
            fUpdateRegistersListView(
                pstTargetInfo->pstDebugInfoFromGui->stTabPageInfo.hListRegisters,
                pstTargetInfo->stPrevBpInfo.dwThreadId);

            // 3. Show disassembly
            fShowDisassembly(pstTargetInfo, pstTargetInfo->stPrevBpInfo.stBpInfo.dwTargetAddr);

            break;
        }

        case DSTATE_SINGLESTEP_BEFORE:
        {
            // Update UI to have updated target information displayed
            
            // 1. Update threads information
            if(!fUpdateThreadsListView(
                    pstTargetInfo->pstDebugInfoFromGui->stTabPageInfo.hListThreads, 
                    pstTargetInfo->phtThreads,
                    pstTargetInfo->stProcessInfo.hThread))
            {
                logerror(pstLogger, L"%s(): fUpdateThreadsListView failed %u", __FUNCTIONW__, GetLastError());

                // TODO: bubble up the error or show MessageBox?
            }

            // 2. Update CPU register values
            // TODO: handle error
            fUpdateRegistersListView(
                pstTargetInfo->pstDebugInfoFromGui->stTabPageInfo.hListRegisters,
                pstTargetInfo->dwSSThreadId);

            // 3. Show disassembly
            fShowDisassembly(pstTargetInfo, pstTargetInfo->dwSSTargetAddr);

            break;
        }

        case DSTATE_WAITFOR_DBGBREAK:
        {
            // show busy mouse cursor?

            break;
        }
    }

    pstTargetInfo->iPrevDebugState = iCurState;
    pstTargetInfo->iDebugState = iNewState;

    // Set menu item states
    vSetMenuItemsState(pstTargetInfo);

    return;
}

void vSetMenuItemsState(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);

    switch(pstTargetInfo->iDebugState)
    {
        case DSTATE_INVALID:
        case DSTATE_EXIT:
        {
            vMiDebugSessionEnd(pstTargetInfo->pstDebugInfoFromGui->hMainMenu);
            break;
        }

        case DSTATE_RUNNING:
        case DSTATE_SINGLESTEP_AFTER:
        {
            vMiDebuggerRunning(pstTargetInfo->pstDebugInfoFromGui->hMainMenu);
            break;
        }

        case DSTATE_DEBUGGING:
        //case DSTATE_BREAKPOINTWAIT:   // same as DSTATE_DEBUGGING
        case DSTATE_SINGLESTEP_BEFORE:
        {
            vMiDebuggerDebugging(pstTargetInfo->pstDebugInfoFromGui->hMainMenu);
            break;
        }

        case DSTATE_WAITFOR_DBGBREAK:
        {
            // TODO: disable all target control menu items?
            break;
        }

        default:
        {
            ASSERT(FALSE);
            break;
        }
    }
}

BOOL fShowDisassembly(PTARGETINFO pstTargetInfo, DWORD dwStartFromTargetAddress)
{
    ASSERT(pstTargetInfo);
    ASSERT(dwStartFromTargetAddress > 0);

    BYTE abTargetCode[60];
    SIZE_T ulBytesRead = 0;

    DASMSTATE stDasmState;
    HWND hEditControl;

    // Read 'x' bytes from process memory

    // Prototyping this for now

    // We want to display 10 instructions; assuming each average length of each 
    // instruction is 6 bytes (just a guess), read 60 bytes from target memory
    if(!ReadProcessMemory(pstTargetInfo->stProcessInfo.hProcess, (LPCVOID)dwStartFromTargetAddress, abTargetCode, sizeof(abTargetCode), &ulBytesRead))
    {
        return FALSE;
    }

    ZeroMemory(&stDasmState, sizeof(stDasmState));

    hEditControl = pstTargetInfo->pstDebugInfoFromGui->stTabPageInfo.hEditDisass;
    CLEAR_EDITCONTROL(hEditControl);

    for(int numInst = 0; numInst < 10; ++numInst)
    {
        if(!fDasmDisassembleOne(&stDasmState, abTargetCode, ulBytesRead, FALSE, dwStartFromTargetAddress, NULL))
        {
            dbgwprintf(L"fDasmDisassembleOne FAILED\n");
            break;
        }

        // Add to edit control
        SendMessage(hEditControl, EM_REPLACESEL, FALSE, (LPARAM)stDasmState.szDisassembledInst);
    }

    return FALSE;
}
