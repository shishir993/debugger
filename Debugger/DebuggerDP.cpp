
#include "Inc\DebuggerDP.h"

BOOL CALLBACK DebuggerDP(HWND hDlg, UINT message, WPARAM /*wParam*/, LPARAM lParam)
{
    RECT rcOwner;
    // RECT rcTabAdjust;

    switch(message)
	{
		case WM_INITDIALOG:
		{
            HWND hTabParent = NULL;

            ASSERT(lParam);

            hTabParent = (HWND)lParam;
            
            // Get owner client rect so that I can fill up
            // the complete space of owner's client area
            RECT rcTemp;
            GetClientRect(hTabParent, &rcOwner);
            CopyRect(&rcTemp, &rcOwner);
            TabCtrl_AdjustRect(hTabParent, FALSE, &rcOwner);

            POINT p;
            p.x = rcOwner.left;
            p.y = rcOwner.top;
            ClientToScreen(hTabParent, &p);

            SetWindowPos(hDlg, HWND_TOP, p.x, p.y, rcOwner.right, rcOwner.bottom, SWP_SHOWWINDOW);
            		
			return TRUE;
		}

        case WM_SIZE:
        {
            break;
        }

		case WM_COMMAND:
		{
			// handle commands from child controls
			//switch(LOWORD(wParam))
			//{

			//}
			return FALSE;
		}

        case WM_CLOSE:
		{
			EndDialog(hDlg, 0);
			return TRUE;
		}

    }// switch(message)
	return FALSE;
}
