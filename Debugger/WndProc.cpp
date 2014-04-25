
#include "Inc\WndProc.h"
#include "Inc\DebuggerDP.h"
#include "Inc\MenuItems.h"
#include "Inc\GuiManager.h"
#include "Inc\Debug.h"

extern PLOGGER pstLogger;
extern HINSTANCE g_hMainInstance;

static HWND hMainTab = NULL;
static HMENU hMainMenu = NULL;

static int iConInHandle = -1;
static int iConOutHandle = -1;
static int iConErrHandle = -1;

static int iCurTabIndex = -1;

// ** File local funtions **
static BOOL fCreateConsoleWindow();

// Menu item handlers
static BOOL fStartDebugSession(HWND hMainWindow, struct _DbgSessionStart *pstSessionInfo, __out DWORD *pdwErrCode);

static BOOL fSendMessageDebugThread(int iCurIndex, DWORD dwMsgToSend, PGUIDBGCOMM pstCommInfo);


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD dwError = ERROR_SUCCESS;

    switch(message)
	{
		case WM_CREATE:
		{
            vWriteLog(pstLogger, L"WM_CREATE");

#if _DEBUG
            fCreateConsoleWindow();
#endif

            if(!fCreateMainTabControl(hWnd, &hMainTab, &dwError))
            {
                vWriteLog(pstLogger, L"%s(): Cannot create main control: %d", __FUNCTIONW__, dwError);
                SendMessage(hWnd, WM_CLOSE, 0, 0);
            }
            else
            {
                hMainMenu = GetMenu(hWnd);
                vMiDebuggerInit(hMainMenu);
            }

            if(!fGuiInitialize(&dwError))
            {
                logerror(pstLogger, L"%s(): Cannot initialize Gui module: %d", __FUNCTIONW__, dwError);
                SendMessage(hWnd, WM_CLOSE, 0, 0);
            }

            return 0;
        }

        case WM_CLOSE:
		{
            vWriteLog(pstLogger, L"WM_CLOSE");
			break;
		}

		case WM_DESTROY:
		{   
            vWriteLog(pstLogger, L"WM_DESTROY");
            vGuiExit();

            // Terminate logger only at the very end
            vTerminateLogger(pstLogger);
			
            PostQuitMessage(0);
			return 0;
		}

        case WM_CTLCOLORSTATIC:
        {
            break;
        }

        case WM_NOTIFY:
        {
            switch(((LPNMHDR)lParam)->code)
            {
                case TCN_SELCHANGE:
                {
                    iCurTabIndex = TabCtrl_GetCurSel(hMainTab);
                    if(iCurTabIndex < 0)
                    {
                        logwarn(pstLogger, L"%s() %d: TabCtrl_GetCurSel() failed %u", __FUNCTIONW__, __LINE__, GetLastError());
                    }
                    return 0;
                }
            }
            break;
        }

        case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
                case IDM_DEBUGPROGRAM:
                {
                    DBG_SESSIONSTART stSessionInfo;

                    // Init struct
                    stSessionInfo.szTargetPath[0] = 0;

                    DialogBoxParam(g_hMainInstance, MAKEINTRESOURCE(IDD_OPENPROGRAM), hWnd, fGetNewProgramDP, (LPARAM)&stSessionInfo);
                    if(stSessionInfo.szTargetPath[0] == 0)
                    {
                        logwarn(pstLogger, L"%s(): Did not get target path to debug from dialog box. Last error = %u", __FUNCTIONW__, GetLastError());
                    }
                    else
                    {
                        fStartDebugSession(hWnd, &stSessionInfo, &dwError);
                    }
                    return 0;
                }

                case IDM_DEBUGPROCESS:
                {
                    DBG_SESSIONSTART stSessionInfo;

                    // Init struct
                    stSessionInfo.dwTargetPID = 0;

                    DialogBoxParam(g_hMainInstance, MAKEINTRESOURCE(IDD_GETPROCID), hWnd, fGetProcIdDP, (LPARAM)&stSessionInfo);
                    if(stSessionInfo.dwTargetPID == 0)
                    {
                        logwarn(pstLogger, L"%s(): Did not get PID to debug from dialog box. Last error = %u", __FUNCTIONW__, GetLastError());
                    }
                    else
                    {
                        fStartDebugSession(hWnd, &stSessionInfo, &dwError);
                    }
                    return 0;
                }

                case IDM_TERMINATETARGET:
                {
                    // TODO: handle error return. show MessageBox?
                    fSendMessageDebugThread(iCurTabIndex, GD_SESS_TERM,  NULL);
                    return 0;
                }

                case IDM_DETACHFROMTARGET:
                {
                    fSendMessageDebugThread(iCurTabIndex, GD_SESS_DETACH,  NULL);
                    return 0;
                }

                case IDM_DUMPANDTERMINATETARGET:
                {
                    fSendMessageDebugThread(iCurTabIndex, GD_SESS_DUMPTERM,  NULL);
                    return 0;
                }

				case IDM_EXITDEBUGGER:
				{
                    vWriteLog(pstLogger, L"IDM_EXITDEBUGGER");

                    // TODO: detach/terminate from all targets being debugged

                    if(!fOnExitDetachTargets())
                    {
                        MessageBox(hWnd, L"Error detaching from targets. See log file.", L"Error", MB_ICONEXCLAMATION|MB_OK);
                    }

                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0;
                }

                case IDM_CONTINUE:
                {
                    fSendMessageDebugThread(iCurTabIndex, GD_MENU_CONTINUE,  NULL);
                    return 0;
                }

                case IDM_SUSPENDALLTHREADS:
                {
                    fSendMessageDebugThread(iCurTabIndex, GD_MENU_SUSPALL,  NULL);
                    return 0;
                }

                case IDM_RESUMEALLTHREADS:
                {
                    fSendMessageDebugThread(iCurTabIndex, GD_MENU_RESALL,  NULL);
                    return 0;
                }

            }// switch(LOWORD...)
            
            break;

        }// case WM_COMMAND

        // ****************************** Messages from Debug thread ******************************
        case DG_SESS_TERM:
        {
            PGUIDBGCOMM pstDbgComm = (PGUIDBGCOMM)lParam;

            ASSERT(pstDbgComm);

            logtrace(pstLogger, L"%s(): DG_SESS_TERM from thread %u", __FUNCTIONW__, pstDbgComm->dwThreadID);
            if(!fGuiRemTab((int)wParam, &dwError))
            {
                logwarn(pstLogger, L"%s(): fGuiRemTab() failed to remove index %d %u", (int)wParam, dwError);
            }

            // Continue even if we failed to remove from hash table (next time we insert, it will simply get overwritten)
            vDeleteTabPage(hMainTab, &pstDbgComm->stTabPageInfo);

            FREEIF_GUIDBGCOMM(pstDbgComm);

            return 0;
        }

    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static BOOL fStartDebugSession(HWND hMainWindow, struct _DbgSessionStart *pstSessionInfo, __out DWORD *pdwErrCode)
{
    ASSERT(ISVALID_HANDLE(hMainWindow));
    ASSERT(pstSessionInfo);

    DWORD dwErrorCode = ERROR_SUCCESS;
    WCHAR szLogMessage[SLEN_LOGLINE];

    HANDLE hEventInitSync = NULL;
    WCHAR szEventNameSync[SLEN_EVENTNAMES];
    PDEBUGINFO pDebugInfo = NULL;

    HANDLE hThreadDebug = NULL;
    DWORD dwThreadId = 0;
    
    DWORD dwWaitResult = WAIT_FAILED;

    if(!fChlMmAlloc((void**)&pDebugInfo, sizeof(DEBUGINFO), &dwErrorCode))
    {
        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): fChlMmAlloc failed: %u", __FUNCTIONW__, dwErrorCode);
        vWriteLog(pstLogger, szLogMessage);
        goto error_return;
    }

    // Debug thread needs to know whether it is debugging a new program or
    // an active process
    if(pstSessionInfo->fDebuggingActiveProcess)
    {
        ASSERT(pstSessionInfo->dwTargetPID > 0);
        pDebugInfo->fDebuggingActiveProcess = TRUE;
        pDebugInfo->dwProcessID = pstSessionInfo->dwTargetPID;
    }
    else
    {
        ASSERT(pstSessionInfo->szTargetPath[0] != 0);
        wcscpy_s(pDebugInfo->szTargetPath, _countof(pDebugInfo->szTargetPath), pstSessionInfo->szTargetPath);
    }

    // Insert new tab page
    if(!fCreateTabPage(hMainTab, &pDebugInfo->stTabPageInfo, &dwErrorCode))
    {
        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): fCreateTabPage failed: %u", __FUNCTIONW__, dwErrorCode);
        vWriteLog(pstLogger, szLogMessage);
        goto error_return;
    }

    iCurTabIndex = pDebugInfo->stTabPageInfo.iTabIndex;

    // Create the init sync event, not signaled to start with
    LoadString(g_hMainInstance, IDS_GUIDBG_SYNC, szEventNameSync, _countof(szEventNameSync));
    hEventInitSync = CreateEvent(NULL, FALSE, FALSE, szEventNameSync);
    if(hEventInitSync == NULL)
    {
        SET_ERRORCODE(dwErrorCode);
        vWriteLog(pstLogger, L"%s(): CreateEvent failed: %u %s", __FUNCTIONW__, dwErrorCode, szEventNameSync);
        goto error_return;
    }

    // Create the debug thread, pass in the required info
    pDebugInfo->hMainWindow = hMainWindow;
    pDebugInfo->hMainMenu = hMainMenu;
    pDebugInfo->fBreakAtMain = pstSessionInfo->fBreakAtMain;
    wcscpy_s(pDebugInfo->szInitSyncEvtName, _countof(pDebugInfo->szInitSyncEvtName), szEventNameSync);

    if( (hThreadDebug = CreateThread(NULL, 0, dwDebugThreadEntry, (LPVOID)pDebugInfo, CREATE_SUSPENDED, &dwThreadId)) == NULL )
    {
        SET_ERRORCODE(dwErrorCode);
        vWriteLog(pstLogger, L"%s(): CreateEvent failed: %u %s", __FUNCTIONW__, dwErrorCode, szEventNameSync);
        goto error_return;
    }

    // Add into map
    if(!fGuiAddTab(iCurTabIndex, dwThreadId, &dwErrorCode))
    {
        logerror(pstLogger, L"Could not insert into tab-thread map: %u", dwErrorCode);
        TerminateThread(hThreadDebug, 0);   // safe to terminate since it is anyway not running
        FREE_HANDLE(hThreadDebug);
        goto error_return;
    }

    // todo: handle return value
    ResumeThread(hThreadDebug);

    dwWaitResult = WaitForSingleObject(hEventInitSync, 5000);
    if( dwWaitResult != WAIT_OBJECT_0 )
    {
        SET_ERRORCODE(dwErrorCode);
        // ASSERT(FALSE);
        vWriteLog(pstLogger, L"%s(): Debug thread(id: %u) failed to signal init sync event", __FUNCTIONW__, dwThreadId);
        // goto error_return;
    }
    FREE_HANDLE(hEventInitSync);
    vWriteLog(pstLogger, L"%s(): Done...", __FUNCTIONW__);
    return TRUE;

    error_return:
    FREE_HANDLE(hEventInitSync);
    IFPTR_FREE(pDebugInfo);
    IFPTR_SETVAL(pdwErrCode, dwErrorCode);
    // remove the tab page
    return FALSE;
}

static BOOL fSendMessageDebugThread(int iCurIndex, DWORD dwMsgToSend, PGUIDBGCOMM pstCommInfo)
{
    ASSERT(iCurIndex >= 0);
    ASSERT(dwMsgToSend >= CUSTOM_GDEVENT_START && dwMsgToSend <= CUSTOM_GDEVENT_END);

    DWORD dwError;
    DWORD dwThreadId;

    DBG_UNREFERENCED_PARAMETER(pstCommInfo);

    if(!fGuiFindTab(iCurTabIndex, &dwThreadId, &dwError))
    {
        logerror(pstLogger, L"Cannot find thread ID for tab index %d", iCurTabIndex);
        return FALSE;
    }

    return PostThreadMessage(dwThreadId, dwMsgToSend, 0, NULL);
}

static BOOL fCreateConsoleWindow()
{
    BOOL fError = TRUE;

    __try
    {
        if(!AllocConsole())
        {
            logerror(pstLogger, L"Could not allocate new console: %d", GetLastError());
        }
        else
        {
            // Thanks to: http://dslweb.nwnexus.com/~ast/dload/guicon.htm and MSDN
            FILE *fp = NULL;
            HANDLE hInputHandle = GetStdHandle(STD_INPUT_HANDLE);
            HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            HANDLE hErrorHandle = GetStdHandle(STD_ERROR_HANDLE);

            if(hInputHandle == INVALID_HANDLE_VALUE || hOutputHandle == INVALID_HANDLE_VALUE || 
                hErrorHandle == INVALID_HANDLE_VALUE)
            {
                __leave;
            }

            if( (iConInHandle = _open_osfhandle((intptr_t)hInputHandle, _O_RDONLY|_O_TEXT)) == -1 )
                __leave;

            fp = _fdopen( iConInHandle, "r" );
            *stdin = *fp;

            if( (iConOutHandle = _open_osfhandle((intptr_t)hOutputHandle, _O_APPEND|_O_TEXT)) == -1 )
                __leave;

            fp = _fdopen( iConOutHandle, "w+" );
            *stdout = *fp;

            if( (iConErrHandle = _open_osfhandle((intptr_t)hErrorHandle, _O_APPEND|_O_TEXT)) == -1 )
                __leave;

            fp = _fdopen( iConErrHandle, "w+" );
            *stderr = *fp;

            fError = FALSE;
        }
    }
    __finally
    {
        if(fError)
        {
            if(iConInHandle != -1) { _close(iConInHandle); iConInHandle = -1; }
            if(iConOutHandle != -1) { _close(iConOutHandle); iConOutHandle = -1; }
            if(iConErrHandle != -1) { _close(iConErrHandle); iConErrHandle = -1; }
            FreeConsole();
        }
    }
    
    return !fError;
}
