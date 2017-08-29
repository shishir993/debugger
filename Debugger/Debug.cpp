
#include "Inc\Debug.h"
#include "Inc\DebugHelpers.h"

extern PLOGGER pstLogger;
extern HINSTANCE g_hMainInstance;

// ** File local functions **
static BOOL fInitialSyncWithGuiThread(const WCHAR *pszSyncEventName);
static BOOL fDebugNewProgram(PTARGETINFO pstTargetInfo);
static BOOL fDebugActiveProcess(PTARGETINFO pstTargetInfo);
static void vOnThisThreadExit(PTARGETINFO *ppstTargetInfo);
static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo);
static BOOL fProcessDebugEventLoop(PTARGETINFO pstTargetInfo);

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
    if(FAILED(CHL_MmAlloc((void**)&pstTargetInfo, sizeof(TARGETINFO), &dwErrCode)))
    {
        vWriteLog(pstLogger, L"%s(): CHL_MmAlloc() failed: %u", __FUNCTIONW__, dwErrCode);
        CHL_MmFree(&lpvArgs);
        return dwErrCode;
    }

    pstTargetInfo->pstDebugInfoFromGui = PDEBUGINFO(lpvArgs);

    if(!fInitialSyncWithGuiThread(PDEBUGINFO(lpvArgs)->szInitSyncEvtName))
    {
        dwErrCode = GetLastError();
        logerror(pstLogger, L"%s(): Could not sync with Gui thread: %u", __FUNCTIONW__, dwErrCode);
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

    pstTargetInfo->iDebugState = DSTATE_RUNNING;
    vSetMenuItemsState(pstTargetInfo);

    // Start from-gui message loop and debug event loop
    fContinueProcessing = TRUE;

    // Execute loop while we have either GUI messages or debug event loop to process
    while(fContinueProcessing)
    {
        // This returns TRUE always, for now
        fProcessGuiMessage(pstTargetInfo);

        // Decide whether to run the debug event loop based on value of iDebugState
        switch(pstTargetInfo->iDebugState)
        {
            case DSTATE_INVALID:
            {
                ASSERT(FALSE);
            }
            // Fall through
            case DSTATE_DEBUGGING:
            //case DSTATE_SINGLESTEP_BEFORE:
            //case DSTATE_BREAKPOINTWAIT:
            {
                continue;
            }

            case DSTATE_EXIT:
            {
                fContinueProcessing = FALSE;
                break;
            }

            // DSTATE_RUNNING
            // DSTATE_WAITFOR_DBGBREAK
            default:
            {
                fContinueProcessing = fProcessDebugEventLoop(pstTargetInfo);
                break;
            }
        }
    }

    vWriteLog(pstLogger, L"%s(): Exiting with ERROR_SUCCESS", __FUNCTIONW__);

    pstTargetInfo->iDebugState = DSTATE_INVALID;
    vSetMenuItemsState(pstTargetInfo);

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
    if(FAILED(CHL_DsCreateHT(&pstTargetInfo->phtThreads, 100, CHL_KT_UINT32, CHL_VT_POINTER, TRUE)))
    {
        logerror(pstLogger, L"CHL_DsCreateHT() failed");
        goto error_return;
    }

	// TODO: Change keytype to handle both 32bit and 64bit addresses in future
    if(FAILED(CHL_DsCreateHT(&pstTargetInfo->phtDllsLoaded, 500, CHL_KT_UINT32, CHL_VT_WSTRING, TRUE)))
    {
        logerror(pstLogger, L"CHL_DsCreateHT() failed");
        goto error_return;
    }

    // Create breakpoints list
    if(!fBpInitialize(&pstTargetInfo->pListBreakpoint))
    {
        logerror(pstLogger, L"fBpInitialize() failed: %u", GetLastError());
        goto error_return;
    }

    return TRUE;

error_return:
    if(pstTargetInfo->phtDllsLoaded)
    {
        CHL_DsDestroyHT(pstTargetInfo->phtDllsLoaded);
    }

    if(pstTargetInfo->phtThreads)
    {
        CHL_DsDestroyHT(pstTargetInfo->phtThreads);
    }

    if(pstTargetInfo->pListBreakpoint)
    {
        fBpTerminate(pstTargetInfo->pListBreakpoint);
    }
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
    if(FAILED(CHL_DsCreateHT(&pstTargetInfo->phtThreads, 100, CHL_KT_UINT32, CHL_VT_POINTER, TRUE)))
    {
        logerror(pstLogger, L"CHL_DsCreateHT() failed");
        goto error_return;
    }

    if(FAILED(CHL_DsCreateHT(&pstTargetInfo->phtDllsLoaded, 500, CHL_KT_UINT32, CHL_VT_WSTRING, TRUE)))
    {
        logerror(pstLogger, L"CHL_DsCreateHT() failed");
        goto error_return;
    }

    // Create breakpoints list
    if(!fBpInitialize(&pstTargetInfo->pListBreakpoint))
    {
        logerror(pstLogger, L"fBpInitialize() failed: %u", GetLastError());
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
    if(pstTargetInfo->phtDllsLoaded)
    {
        CHL_DsDestroyHT(pstTargetInfo->phtDllsLoaded);
    }

    if(pstTargetInfo->phtThreads)
    {
        CHL_DsDestroyHT(pstTargetInfo->phtThreads);
    }

    if(pstTargetInfo->pListBreakpoint)
    {
        fBpTerminate(pstTargetInfo->pListBreakpoint);
    }

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

    // 20140424: Changed to use async send so that we do not introduce deadlock in the case
    // where Gui is waiting for this thread to exit before its own exit
    // The debug thread anyway doesn't care whether Gui processed this message or not
    // (the debug thread can't really do anything useful even when SendMessage fails).
    //
    // Also, do not notify Gui if this exit is from a detach that is due to debugger itself exiting
    // because this message could be processed by Gui after it processes WM_CLOSE and WM_DESTROY which
    // is disastrous since all cleanup would have been finished by then.
    if(!plocal->fDetachOnDebuggerExit)
    {
        // Notify Gui thread of exit
        if(FAILED(CHL_MmAlloc((void**)&pstGuiComm, sizeof(GUIDBGCOMM), &dwError)))
        {
            logerror(pstLogger, L"%s(): CHL_MmAlloc() failed %u", __FUNCTIONW__, dwError);
        }
        else
        {
            pstGuiComm->fFreeThis = TRUE;
            pstGuiComm->dwThreadID = GetCurrentThreadId();
            memcpy(&pstGuiComm->stTabPageInfo, &plocal->pstDebugInfoFromGui->stTabPageInfo, sizeof(TABPAGEINFO));

            SendNotifyMessage(
                plocal->pstDebugInfoFromGui->hMainWindow, 
                DG_SESS_TERM, 
                (WPARAM)plocal->pstDebugInfoFromGui->stTabPageInfo.iTabIndex, 
                (LPARAM)pstGuiComm);
        }
    }

    // Close handles
    UnmapViewOfFile(plocal->hFileMapView);
    CloseHandle(plocal->hFileMapObj);
    CloseHandle(plocal->stProcessInfo.hFile);

    // free threads table
    if(plocal->phtThreads && !CHL_DsDestroyHT(plocal->phtThreads))
    {
        logerror(pstLogger, L"(%s): Unable to destroy DLL hash table", __FUNCTIONW__);
    }

    // free Dlls table
    if(plocal->phtDllsLoaded && !CHL_DsDestroyHT(plocal->phtDllsLoaded))
    {
        logerror(pstLogger, L"(%s): Unable to destroy DLL hash table", __FUNCTIONW__);
    }

    // free breakpoint linked list
    if(plocal->pListBreakpoint && !fBpTerminate(plocal->pListBreakpoint))
    {
        logerror(pstLogger, L"(%s): Unable to destroy breakpoint linked list %u", __FUNCTIONW__, GetLastError());
    }

    // Free the memory occupied by DEBUGINFO
    CHL_MmFree((void**)&plocal->pstDebugInfoFromGui);

    // Finally, free the memory occupied by TARGETINFO
    CHL_MmFree((void**)&plocal);

    return;
}

// Process any messages posted to this thread by the Gui thread
// If this function returns FALSE, then it means that this Debug thread
// must exit. Right now, FALSE is returned only when detaching from target
static BOOL fProcessGuiMessage(PTARGETINFO pstTargetInfo)
{
    ASSERT(pstTargetInfo);

    MSG msg;

    PGUIDBGCOMM pstGuiComm = NULL;

    while( PeekMessage(&msg, (HWND)-1, CUSTOM_GDEVENT_START, CUSTOM_GDEVENT_END, PM_REMOVE) )
    {
        pstGuiComm = (PGUIDBGCOMM)msg.lParam;
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
                // Re-insert breakpoint if it is a multi-hit BP AND we did not enter debugging state
                // from a DebugBreak caused breakpoint exception
                if(pstTargetInfo->iDebugState == DSTATE_BREAKPOINTWAIT /* sme as DSTATE_DEBUGGING */ )
                {
                    if(!fReInsertBPIf(pstTargetInfo, &pstTargetInfo->stPrevBpInfo))
                    {
                        dbgwprintf(L"()%s: Failed to remove breakpoint", __FUNCTIONW__);
                        logerror(pstLogger, L"()%s: Failed to remove breakpoint", __FUNCTIONW__);
                        break;
                    }

                    // Continue debug event
                    if(!ContinueDebugEvent(pstTargetInfo->dwPID, pstTargetInfo->stPrevBpInfo.dwThreadId, DBG_CONTINUE))
                    {
                        dbgwprintf(L"%s(): ContinueDebugEvent failed %u\n", __FUNCTIONW__, GetLastError());
                        logerror(pstLogger, L"%s(): ContinueDebugEvent failed %u\n", __FUNCTIONW__, GetLastError());
                    }

                    ZeroMemory(&pstTargetInfo->stPrevBpInfo, sizeof(pstTargetInfo->stPrevBpInfo));
                }
                else if(pstTargetInfo->iDebugState == DSTATE_SINGLESTEP_BEFORE)
                {
                    // Clear TF so that we continue executing target
                    if(!fClearTrapFlag(pstTargetInfo->phtThreads, pstTargetInfo->dwSSThreadId))
                    {
                        logerror(pstLogger, L"%s(): Unable to clear TF in thread ID %u", __FUNCTIONW__, pstTargetInfo->dwSSThreadId);
                        // TODO: messagebox
                        break;
                    }

                    // Continue debug event
                    if(!ContinueDebugEvent(pstTargetInfo->dwPID, pstTargetInfo->dwSSThreadId, DBG_CONTINUE))
                    {
                        dbgwprintf(L"%s(): ContinueDebugEvent failed after SS %u\n", __FUNCTIONW__, GetLastError());
                        logerror(pstLogger, L"%s(): ContinueDebugEvent failed after SS %u\n", __FUNCTIONW__, GetLastError());
                    }

                    pstTargetInfo->dwSSTargetAddr = pstTargetInfo->dwSSThreadId = 0;
                }

                // Change debugger state
                vDebuggerStateChange(pstTargetInfo, DSTATE_RUNNING);

                break;
            }

            case GD_MENU_STEPINTO:
            {
                if(pstTargetInfo->iDebugState == DSTATE_BREAKPOINTWAIT)
                {
                    if(!fSetTrapFlag(pstTargetInfo->phtThreads, pstTargetInfo->stPrevBpInfo.dwThreadId))
                    {
                        logerror(pstLogger, L"%s(): Unable to set TF in thread ID %u", __FUNCTIONW__, pstTargetInfo->stPrevBpInfo.dwThreadId);
                        // TODO: messagebox
                        break;
                    }

                    if(!ContinueDebugEvent(pstTargetInfo->dwPID, pstTargetInfo->stPrevBpInfo.dwThreadId, DBG_CONTINUE))
                    {
                        dbgwprintf(L"%s(): ContinueDebugEvent failed %u\n", __FUNCTIONW__, GetLastError());
                        logerror(pstLogger, L"%s(): ContinueDebugEvent failed %u\n", __FUNCTIONW__, GetLastError());
                        break;
                    }

                    ZeroMemory(&pstTargetInfo->stPrevBpInfo, sizeof(pstTargetInfo->stPrevBpInfo));
                }
                else if(pstTargetInfo->iDebugState == DSTATE_SINGLESTEP_BEFORE)
                {
                    if(!fSetTrapFlag(pstTargetInfo->phtThreads, pstTargetInfo->dwSSThreadId))
                    {
                        logerror(pstLogger, L"%s(): Unable to set TF in thread ID %u", __FUNCTIONW__, pstTargetInfo->stPrevBpInfo.dwThreadId);
                        // TODO: messagebox
                        break;
                    }

                    if(!ContinueDebugEvent(pstTargetInfo->dwPID, pstTargetInfo->dwSSThreadId, DBG_CONTINUE))
                    {
                        dbgwprintf(L"%s(): ContinueDebugEvent failed %u\n", __FUNCTIONW__, GetLastError());
                        logerror(pstLogger, L"%s(): ContinueDebugEvent failed %u\n", __FUNCTIONW__, GetLastError());
                        break;
                    }
                }
                else
                {
                    ASSERT(FALSE);
                    break;
                }

                vDebuggerStateChange(pstTargetInfo, DSTATE_SINGLESTEP_AFTER);

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
                if(!DebugBreakProcess(pstTargetInfo->stProcessInfo.hProcess))
                {
                    logerror(pstLogger, L"%s(): DebugBreakProcess failed %u", GetLastError());
                    break;
                    // TODO: show messagebox
                }

                // wait for breakpoint exception
                vDebuggerStateChange(pstTargetInfo, DSTATE_WAITFOR_DBGBREAK);
                
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
                    logerror(pstLogger, L"%s(): TerminateProcess() failed %u", __FUNCTIONW__, GetLastError());
                }
                break;
            }

            case GD_SESS_DETACH:
            {
                logtrace(pstLogger, L"%s(): Received GD_SESS_DETACH", __FUNCTIONW__);
                if(!DebugActiveProcessStop(pstTargetInfo->dwPID))
                {
                    MessageBox(pstTargetInfo->pstDebugInfoFromGui->hMainWindow, L"Cannot detach from target. See log file.", L"Error", MB_ICONERROR);
                    logerror(pstLogger, L"%s(): DebugActiveProcessStop() failed %u", __FUNCTIONW__, GetLastError());
                }
                else
                {
                    logtrace(pstLogger, L"%s(): DETACH successful", __FUNCTIONW__);
                    pstTargetInfo->iPrevDebugState = pstTargetInfo->iDebugState;
                    pstTargetInfo->iDebugState = DSTATE_EXIT;
                }

                if(pstGuiComm)
                {
                    pstTargetInfo->fDetachOnDebuggerExit = pstGuiComm->GD_fDetachOnDebuggerExit;
                    FREEIF_GUIDBGCOMM(pstGuiComm);
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

                    // TODO: clean up this code!

                    // TODO: Handle different values of dwContinueStatus

                    if(dwContinueStatus == DBG_CONTCUSTOM_ABORT)
                    {
                        // TODO: TerminateProcess without giving it a chance to execute another instruction
                        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
                        if(!TerminateProcess(pstTargetInfo->stProcessInfo.hProcess, EXITCODE_TARGET_TERM))
                        {
                            logerror(pstLogger, L"%s(): TerminateProcess() failed %u", __FUNCTIONW__, GetLastError());

                            // Return FALSE to quit debug thread so that it will kill the target
                            return FALSE;
                        }
                    }
                    else
                    {
                        /*
                         * This means we hit a breakpoint or we are single stepping.
                         * Either case, we must enter the debugging mode without
                         * giving a chance for the target process to resume execution.
                         */

                        // Check the iDebugState
                        if(pstTargetInfo->iDebugState != DSTATE_BREAKPOINTWAIT /* same as DSTATE_DEBUGGING */
                            && pstTargetInfo->iDebugState != DSTATE_SINGLESTEP_BEFORE)
                        {
                            ASSERT(dwContinueStatus == DBG_CONTINUE || dwContinueStatus == DBG_EXCEPTION_NOT_HANDLED);

                            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);
                        }
                        else
                        {
                            // for now, show that we have hit a breakpoint and return
                            dbgwprintf(L"Entering debugging mode due to exception(0x%08x) at 0x%p\n", de.u.Exception.ExceptionRecord.ExceptionCode, de.u.Exception.ExceptionRecord.ExceptionAddress);
                        }
                    }
                
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

#if 0
    WCHAR wsExString[SLEN_EXCEPTION_NAME];
    WCHAR wsExceptionMessage[SLEN_COMMON64];


    wprintf(
        L"Exception 0x%08X at 0x%p\n", 
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
                L"%s Exception(FirstChance) at: 0x%p", 
                wsExString, 
                lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);
        }
        else
        {
            swprintf_s(
                wsExceptionMessage, 
                _countof(wsExceptionMessage), 
                L"%s Exception at: 0x%p", 
                wsExString, 
                lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);
        }

        MessageBox(NULL, wsExceptionMessage, L"Exception Raised", MB_ICONSTOP|MB_OK);
    }
    
    IFPTR_SETVAL(pdwContinueStatus, DBG_EXCEPTION_NOT_HANDLED);
#else
    
    switch(dwExCode)
    {
        case EXCEPTION_BREAKPOINT:
        {
            logtrace(pstLogger, L"EXCEPTION_BREAKPOINT in %u at %p", lpDebugEvent->dwThreadId, lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);

            dbgwprintf(L"EXCEPTION_BREAKPOINT in %u at %p\n", lpDebugEvent->dwThreadId, lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);

            fHandleExceptionBreakpoint(pstTargetInfo, pdwContinueStatus);
            break;
        }

        case EXCEPTION_SINGLE_STEP:
        {
            logtrace(pstLogger, L"EXCEPTION_SINGLE_STEP in %u at %p", lpDebugEvent->dwThreadId, lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);

            dbgwprintf(L"EXCEPTION_SINGLE_STEP in %u at %p\n", lpDebugEvent->dwThreadId, lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress);

            if(pstTargetInfo->iDebugState != DSTATE_SINGLESTEP_AFTER)
            {
                // Should never happen (?)
                break;
            }

            pstTargetInfo->dwSSThreadId = lpDebugEvent->dwThreadId;
            pstTargetInfo->dwSSTargetAddr = (DWORD)lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress;

            if(!fSetTrapFlag(pstTargetInfo->phtThreads, pstTargetInfo->dwSSThreadId))
            {
                logerror(pstLogger, L"%s(): Unable to set TF in thread ID %u", __FUNCTIONW__, pstTargetInfo->dwSSThreadId);
                // TODO: messagebox
            }
            else
            {
                vDebuggerStateChange(pstTargetInfo, DSTATE_SINGLESTEP_BEFORE);
            }

            *pdwContinueStatus = DBG_CONTINUE;

            break;
        }

        default:
        {
            vSetContinueStatusFromUser(
                lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode, 
                (DWORD)lpDebugEvent->u.Exception.ExceptionRecord.ExceptionAddress,
                lpDebugEvent->u.Exception.dwFirstChance,
                pdwContinueStatus);
            break;
        }
    }

#endif

    // Take action depending on value of iDebugState and pdwContinueStatus
    switch(*pdwContinueStatus)
    {
        case DBG_CONTCUSTOM_ABORT:
        {
            // Nothing to do. Cannot TerminateProcess here because we must call ContinueDebugEvent first
            break;
        }

        case DBG_CONTCUSTOM_BREAK:
        {
            vDebuggerStateChange(pstTargetInfo, DSTATE_DEBUGGING);
            
            // Doesn't matter because we will not call ContinuDebugEvent now
            // so that the target process does not resume execution
            *pdwContinueStatus = 0;
            break;
        }

        default:
        {
            
        }
        
    }

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
    BPINFO stBpInfo;

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
        wprintf(L"Process create: [%u] : %s\nBaseAddr: 0x%p\nStartAddr: 0x%p\nThreadLocal: 0x%p\n", 
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

        // Get some additional info about target process's image
        if(FAILED(CHL_GnCreateMemMapOfFile(lpDebugEvent->u.CreateProcessInfo.hFile, 0, &pstTargetInfo->hFileMapObj, &pstTargetInfo->hFileMapView)))
        {
            logerror(pstLogger, L"%s(): CHL_GnCreateMemMapOfFile() failed %u", __FUNCTIONW__, GetLastError());
            goto error_return;
        }

        if(FAILED(CHL_PsGetNtHeaders(pstTargetInfo->hFileMapView, &pstTargetInfo->pstNtHeaders)))
        {
            logerror(pstLogger, L"%s(): CHL_PsGetNtHeaders() failed %u", __FUNCTIONW__, GetLastError());
            goto error_return;
        }

        if(FAILED(CHL_PsGetPtrToCode(
                (DWORD)pstTargetInfo->hFileMapView, 
                pstTargetInfo->pstNtHeaders, 
                &pstTargetInfo->dwCodeStart, 
                &pstTargetInfo->dwCodeSize, 
                &pstTargetInfo->dwCodeSecVirtAddr)))
        {
            logerror(pstLogger, L"%s(): CHL_PsGetPtrToCode() failed %u", __FUNCTIONW__, GetLastError());
            goto error_return;
        }

        // **** If break at main, insert breakpoint ****
        if(pstTargetInfo->pstDebugInfoFromGui->fBreakAtMain)
        {
            ZeroMemory(&stBpInfo, sizeof(stBpInfo));
            stBpInfo.iBpType = BPTYPE_ASMLEVEL | BPTYPE_USERSINGLEHIT;
            stBpInfo.dwTargetAddr = (DWORD)pstTargetInfo->stProcessInfo.lpStartAddress;

            // TODO: take the BP id??
            if(!fBpInsert(pstTargetInfo->pListBreakpoint, &stBpInfo, pstTargetInfo, NULL))
            {
                // TODO: show message to user
                logerror(
                    pstLogger, 
                    L"%s(): fBpInsert() failed %u. Could not place BP at 0x%p.", 
                    __FUNCTIONW__, 
                    GetLastError(), 
                    (DWORD)pstTargetInfo->stProcessInfo.lpStartAddress);
            }
        }
    }
    else
    {
        // We should receive the create process only once per debugging session
        // since we have specified DEBUG_ONLY_THIS_PROCESS flag
        ASSERT(FALSE);
    }

    ++(pstTargetInfo->nCurThreads);
    ++(pstTargetInfo->nTotalThreads);
    ++(pstTargetInfo->nTotalProcesses);

    // Close handle to the image file, we do not need here... yet
    // CloseHandle(lpDebugEvent->u.CreateProcessInfo.hFile);

    // Close handle when thread exits

    return TRUE;

error_return:
    return FALSE;
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
        wprintf(L"DLL Load: [0x%p] : %s\n", lpDebugEvent->u.LoadDll.lpBaseOfDll, wsImageName);
    }

    // Insert into the DLLs loaded hashtable
    if(FAILED(CHL_DsInsertHT(pstTargetInfo->phtDllsLoaded, lpDebugEvent->u.LoadDll.lpBaseOfDll, sizeof(DWORD), wsImageName, 
        CONV_BYTES_wcsnlen_s(wsImageName, SLEN_MAXPATH))))
    {
        logerror(pstLogger, L"Unable to insert 0x%p:%s into hash", *((DWORD*)lpDebugEvent->u.LoadDll.lpBaseOfDll), wsImageName);
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
    int outValSize = sizeof(pws);

    if(FAILED(CHL_DsFindHT(pstTargetInfo->phtDllsLoaded, lpDebugEvent->u.UnloadDll.lpBaseOfDll, sizeof(DWORD), &pws, &outValSize, TRUE)))
    {
        wprintf(L"DLL Unload : [0x%p] : Image name not found\n", lpDebugEvent->u.UnloadDll.lpBaseOfDll);
    }
    else
    {
        wprintf(L"DLL Unload : [0x%p] : %s\n", lpDebugEvent->u.UnloadDll.lpBaseOfDll, pws);
        CHL_DsRemoveHT(pstTargetInfo->phtDllsLoaded, lpDebugEvent->u.UnloadDll.lpBaseOfDll, sizeof(DWORD));
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

    if(FAILED(CHL_MmAlloc((void**)&pwsDebugString, lpDebugEvent->u.DebugString.nDebugStringLength * sizeof(WCHAR), NULL)))
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

            wprintf(L"OutputDebugString: [%u][%s]\n", bytesRead, pwsDebugString);
        }
        CHL_MmFree((void**)&pwsDebugString);
    }

    return TRUE;
}
