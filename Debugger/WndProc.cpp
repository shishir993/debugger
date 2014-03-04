
#include "Inc\WndProc.h"

LRESULT CALLBACK WndProc(HWND hMainWindow, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch(message)
	{
		case WM_CREATE:
		{
            return 0;
        }

        case WM_CLOSE:
		{
			break;
		}

		case WM_DESTROY:
		{
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
				case IDM_EXITDEBUGGER:
				{
                    SendMessage(hMainWindow, WM_DESTROY, 0, 0);
                    return 0;
                }

            }// switch(LOWORD...)
            
            break;

        }// case WM_COMMAND
    
    }

    return DefWindowProc(hMainWindow, message, wParam, lParam);
}
