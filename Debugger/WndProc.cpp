
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

// Menu item handlers
static BOOL fOnDebugProgram(HWND hMainWindow, __out DWORD *pdwErrCode);
static BOOL fCreateConsoleWindow();

LRESULT CALLBACK WndProc(HWND hMainWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szLogMessage[SLEN_LOGLINE];

    PDEBUGINFO pDebugInfo = NULL;

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
                    fOnDebugProgram(hMainWindow, &dwError);
                    return 0;
                }

				case IDM_EXITDEBUGGER:
				{
                    vWriteLog(pstLogger, L"IDM_EXITDEBUGGER");
                    SendMessage(hMainWindow, WM_CLOSE, 0, 0);
                    return 0;
                }

            }// switch(LOWORD...)
            
            break;

        }// case WM_COMMAND
    }

    return DefWindowProc(hMainWindow, message, wParam, lParam);
}

static BOOL fOnDebugProgram(HWND hMainWindow, __out DWORD *pdwErrCode)
{
    ASSERT(ISVALID_HANDLE(hMainWindow));

    DWORD dwErrorCode = ERROR_SUCCESS;
    WCHAR szLogMessage[SLEN_LOGLINE];

    HANDLE hFile = NULL;
    WCHAR szFilters[] = L"Executables\0*.exe\0\0";

    HANDLE hEventInitSync = NULL;
    WCHAR szEventNameSync[SLEN_EVENTNAMES];
    PDEBUGINFO pDebugInfo = NULL;

    HANDLE hThreadDebug = NULL;
    DWORD dwThreadId = 0;

    if(!fChlMmAlloc((void**)&pDebugInfo, sizeof(DEBUGINFO), &dwErrorCode))
    {
        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): fChlMmAlloc failed: %u", __FUNCTIONW__, dwErrorCode);
        vWriteLog(pstLogger, szLogMessage);
        goto error_return;
    }

    // Get the path to the target program to be debugged
    if(!fGuiGetOpenFilename(hMainWindow, szFilters, &pDebugInfo->pszTargetPath, &dwErrorCode))
    {
        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): fGuiGetOpenFilename failed: %u", __FUNCTIONW__, dwErrorCode);
        vWriteLog(pstLogger, szLogMessage);
        goto error_return;
    }

    ASSERT(pDebugInfo->pszTargetPath);

    // Check if the image file exists and can be read
    hFile = CreateFile(pDebugInfo->pszTargetPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(hMainWindow, L"Chosen target path not found or access denied. See log file.", L"Error", MB_ICONEXCLAMATION);

        SET_ERRORCODE(dwErrorCode);
        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): CreateFile failed: %u", __FUNCTIONW__, dwErrorCode);
        vWriteLog(pstLogger, szLogMessage);
        goto error_return;
    }
    CloseHandle(hFile);

    // Insert new tab page
    if(!fCreateTabPage(hMainTab, &pDebugInfo->stTabPageInfo, &dwErrorCode))
    {
        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): fCreateTabPage failed: %u", __FUNCTIONW__, dwErrorCode);
        vWriteLog(pstLogger, szLogMessage);
        goto error_return;
    }

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
    pDebugInfo->fDebuggingActiveProcess = FALSE;
    pDebugInfo->pszInitSyncEvtName = szEventNameSync;
    if( (hThreadDebug = CreateThread(NULL, 0, dwDebugThreadEntry, (LPVOID)pDebugInfo, 0, &dwThreadId)) == NULL )
    {
        SET_ERRORCODE(dwErrorCode);
        vWriteLog(pstLogger, L"%s(): CreateEvent failed: %u %s", __FUNCTIONW__, dwErrorCode, szEventNameSync);
        goto error_return;
    }

    if( WaitForSingleObject(hEventInitSync, 5000) != WAIT_OBJECT_0 )
    {
        ASSERT(FALSE);
        vWriteLog(pstLogger, L"%s(): Debug thread(id: %u) failed to signal init sync event", __FUNCTIONW__, dwThreadId);
        goto error_return;
    }
    vWriteLog(pstLogger, L"%s(): Done...", __FUNCTIONW__);
    return TRUE;

    error_return:
    IFPTR_FREE(pDebugInfo);
    if(pdwErrCode) { *pdwErrCode = dwErrorCode; }
    return FALSE;
}

static BOOL fCreateConsoleWindow()
{
    BOOL fError = TRUE;

    __try
    {
        if(!AllocConsole())
        {
            DWORD lasterror  = GetLastError();
            MessageBox(NULL, L"Could not allocate new console", L"Error", MB_OK);
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
