
#include "Inc\WndProc.h"
#include "Inc\DebuggerDP.h"
#include "Inc\MenuItems.h"
#include "Inc\GuiManager.h"
#include "Inc\Debug.h"

extern PLOGGER pstLogger;
extern HINSTANCE g_hMainInstance;

struct _DbgSessionStart {
    BOOL fDebuggingProcess;
    DWORD dwTargetPID;
    WCHAR *pszTargetPath;
};

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

LRESULT CALLBACK WndProc(HWND hMainWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szLogMessage[SLEN_LOGLINE];

    switch(message)
	{
		case WM_CREATE:
		{
            vWriteLog(pstLogger, L"WM_CREATE");
            fCreateConsoleWindow();
            if(!fCreateMainTabControl(hMainWindow, &hMainTab, &dwError))
            {
                vWriteLog(pstLogger, L"%s(): Cannot create main control: %d", __FUNCTIONW__, dwError);
                SendMessage(hMainWindow, WM_CLOSE, 0, 0);
            }
            else
            {
                hMainMenu = GetMenu(hMainWindow);
                vMiDebuggerInit(hMainMenu);
            }

            if(!fGuiInitialize(&dwError))
            {
                logerror(pstLogger, L"%s(): Cannot initialize Gui module: %d", __FUNCTIONW__, dwError);
                SendMessage(hMainWindow, WM_CLOSE, 0, 0);
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

        case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
                case IDM_DEBUGPROGRAM:
                {
                    HANDLE hFile = NULL;
                    WCHAR szFilters[] = L"Executables\0*.exe\0\0";
                    WCHAR szTargetPath[SLEN_MAXPATH];

                    struct _DbgSessionStart stInfo;

                    // Get the path to the target program to be debugged
                    if(!fGuiGetOpenFilename(hMainWindow, szFilters, szTargetPath, sizeof(szTargetPath), &dwError))
                    {
                        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): fGuiGetOpenFilename failed: %u", __FUNCTIONW__, dwError);
                        vWriteLog(pstLogger, szLogMessage);
                        return 0;
                    }

                    // Check if the image file exists and can be read
                    hFile = CreateFile(szTargetPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if(hFile == INVALID_HANDLE_VALUE)
                    {
                        MessageBox(hMainWindow, L"Chosen target path not found or access denied. See log file.", L"Error", MB_ICONEXCLAMATION);

                        SET_ERRORCODE(dwError);
                        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): CreateFile failed: %u", __FUNCTIONW__, dwError);
                        vWriteLog(pstLogger, szLogMessage);
                        return 0;
                    }
                    CloseHandle(hFile);

                    stInfo.fDebuggingProcess = FALSE;
                    stInfo.pszTargetPath = szTargetPath;
                    fStartDebugSession(hMainWindow, &stInfo, &dwError);
                    return 0;
                }

                case IDM_DEBUGPROCESS:
                {
                    struct _DbgSessionStart stInfo;

                    // Init struct
                    stInfo.dwTargetPID = 0;

                    DialogBoxParam(g_hMainInstance, MAKEINTRESOURCE(IDD_GETPROCID), hMainWindow, fGetProcIdDP, (LPARAM)&stInfo.dwTargetPID);
                    if(stInfo.dwTargetPID <= 0)
                    {
                        logwarn(pstLogger, L"%s(): Did not get PID to debug from dialog box. Last error = %u", __FUNCTIONW__, GetLastError());
                    }
                    else
                    {
                        fStartDebugSession(hMainWindow, &stInfo, &dwError);
                    }
                    return 0;
                }

				case IDM_EXITDEBUGGER:
				{
                    vWriteLog(pstLogger, L"IDM_EXITDEBUGGER");
                    SendMessage(hMainWindow, WM_CLOSE, 0, 0);
                    return 0;
                }

                case IDM_SUSPENDALLTHREADS:
                {
                    DWORD dwThreadId = 0;

                    if(!fGuiFindTab(iCurTabIndex, &dwThreadId, &dwError))
                    {
                        logerror(pstLogger, L"Cannot find thread ID for tab index %d", iCurTabIndex);

                        // todo: show messagebox?
                    }
                    else
                    {
                        PostThreadMessage(dwThreadId, GD_MENU_SUSPALL, 0, NULL);
                    }

                    return 0;
                }

                case IDM_RESUMEALLTHREADS:
                {
                    DWORD dwThreadId = 0;

                    if(!fGuiFindTab(iCurTabIndex, &dwThreadId, &dwError))
                    {
                        logerror(pstLogger, L"Cannot find thread ID for tab index %d", iCurTabIndex);

                        // todo: show messagebox?
                    }
                    else
                    {
                        PostThreadMessage(dwThreadId, GD_MENU_RESALL, 0, NULL);
                    }
                    
                    return 0;
                }

            }// switch(LOWORD...)
            
            break;

        }// case WM_COMMAND
    }

    return DefWindowProc(hMainWindow, message, wParam, lParam);
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
    if(pstSessionInfo->fDebuggingProcess)
    {
        ASSERT(pstSessionInfo->dwTargetPID > 0);
        pDebugInfo->fDebuggingActiveProcess = TRUE;
        pDebugInfo->dwProcessID = pstSessionInfo->dwTargetPID;
    }
    else
    {
        ASSERT(pstSessionInfo->pszTargetPath);
        wcscpy_s(pDebugInfo->szTargetPath, _countof(pDebugInfo->szTargetPath), pstSessionInfo->pszTargetPath);
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
