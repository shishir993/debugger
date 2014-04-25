
#include "Inc\Common.h"
#include "Inc\WndProc.h"
#include "Inc\UICommon.h"
#include "Inc\Logger.h"

#define WNDSIZE_WIDTH       1024
#define WNDSIZE_HEIGHT      768

HINSTANCE g_hMainInstance;

LOGGER StLogger;
PLOGGER pstLogger = &StLogger;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    HWND g_hMainWnd;
    MSG MainWndMsg;
	WNDCLASS MainWndClass;

	WCHAR szAppName[SLEN_COMMON64];

    int iScreenX, iScreenY, iWndX, iWndY, iWidth, iHeight;
	RECT rcMainWnd;

    DBG_UNREFERENCED_PARAMETER(szCmdLine);
    DBG_UNREFERENCED_PARAMETER(hPrevInstance);

    // First, initialize logger
    if(!fInitializeLogger(L"OSD_Log.txt", &StLogger))
    {
        MessageBox(NULL, L"Cannot initialize logger", L"Error", MB_ICONEXCLAMATION);
        return CE_WMAIN_ERROR;
    }

	g_hMainInstance = hInstance;

    LoadString(GetModuleHandle(NULL), IDS_PGMNAME, szAppName, _countof(szAppName));

	// UI Window Class
	MainWndClass.style		 = CS_HREDRAW | CS_VREDRAW;
	MainWndClass.lpfnWndProc = WndProc;
	MainWndClass.cbClsExtra	 = 0;
	MainWndClass.cbWndExtra  = 0;
	MainWndClass.hInstance	 = g_hMainInstance;
	MainWndClass.hIcon		 = NULL;
	MainWndClass.hCursor	 = LoadCursor(NULL, IDC_ARROW);
	MainWndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	MainWndClass.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
	MainWndClass.lpszClassName = szAppName;

	if(!RegisterClass(&MainWndClass))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"), szAppName, MB_ICONERROR);
		return CE_WMAIN_ERROR;
	}

    // Initialize common controls
	InitCommonControls();

	// Create the main window
	g_hMainWnd = CreateWindow(szAppName,		// class name
						szAppName,				// caption
						WS_CAPTION | 
						WS_MINIMIZEBOX |
						WS_SYSMENU,				// window style
						CW_USEDEFAULT,			// initial X position
						CW_USEDEFAULT,			// initial Y position
						WNDSIZE_WIDTH,					// initial X size
						WNDSIZE_HEIGHT,					// initial Y size
						NULL,					// parent window handle
						NULL,					// window menu handle
						g_hMainInstance,		// program instance handle
						NULL);

	// exit if window was not created
	if( !g_hMainWnd )
	{
		MessageBox(0, L"Main Window creation error. Cannot continue.", 0, 0);
		return CE_WMAIN_ERROR;
	}

	// centre the main window in the screen

	// get the screen co-ordinates
	iScreenX = GetSystemMetrics(SM_CXSCREEN);
	iScreenY = GetSystemMetrics(SM_CYSCREEN);

	// get window rect and calculate the main window dimensions
	GetWindowRect(g_hMainWnd, &rcMainWnd);
	iWidth = rcMainWnd.right - rcMainWnd.left;
	iHeight = rcMainWnd.bottom - rcMainWnd.top;

	// calculate the new co-ordinates for the main window
	iWndX = iScreenX / 2 - iWidth / 2;
	iWndY = iScreenY / 2 - iHeight / 2;
	
	MoveWindow(g_hMainWnd, iWndX, iWndY, iWidth, iHeight, FALSE);

    ShowWindow(g_hMainWnd, iCmdShow);
	UpdateWindow(g_hMainWnd);

	while( GetMessage(&MainWndMsg, NULL, 0, 0) )
	{
		TranslateMessage(&MainWndMsg);
		DispatchMessage(&MainWndMsg);
	}
	
	return MainWndMsg.wParam;
}
