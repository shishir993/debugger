
#include "Inc\WndProc.h"
#include "Inc\DebuggerDP.h"

extern LOGGER g_stLogger;

static HWND hMainTab = NULL;

LRESULT CALLBACK WndProc(HWND hMainWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD dwError;

    switch(message)
	{
		case WM_CREATE:
		{
            vWriteLog(&g_stLogger, L"WM_CREATE");
            if(!fCreateMainTabControl(hMainWindow, &hMainTab, &dwError))
            {

            }

            return 0;
        }

        case WM_CLOSE:
		{
            vWriteLog(&g_stLogger, L"WM_CLOSE");
			break;
		}

		case WM_DESTROY:
		{   
            vWriteLog(&g_stLogger, L"WM_DESTROY");
            vTerminateLogger(&g_stLogger);
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
                    // Calculate some common X, Y, Width and Height values
                    //CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_UNDERTAB), hMainTab, DebuggerDP, (LPARAM)hTab);
                    TABPAGEINFO stTabChildren;

                    fCreateTabPage(hMainTab, &stTabChildren, &dwError);
                    return 0;
                }

				case IDM_EXITDEBUGGER:
				{
                    vWriteLog(&g_stLogger, L"IDM_EXITDEBUGGER");
                    SendMessage(hMainWindow, WM_CLOSE, 0, 0);
                    return 0;
                }

            }// switch(LOWORD...)
            
            break;

        }// case WM_COMMAND
    }

    return DefWindowProc(hMainWindow, message, wParam, lParam);
}
