
#ifndef _GUIMANAGER_H
#define _GUIMANAGER_H

#include "Common.h"
#include <Psapi.h>

#include "UICommon.h"
#include "CHelpLibDll.h"

BOOL fGuiInitialize(__out DWORD *pdwErrCode);
void vGuiExit();
BOOL fGuiAddTab(int tabIndex, DWORD threadId, __out DWORD *pdwErrCode);
BOOL fGuiRemTab(int tabIndex, __out DWORD *pdwErrCode);
BOOL fGuiFindTab(int tabIndex, __out DWORD *pdwThreadId, __out DWORD *pdwErrCode);

BOOL fGuiGetOpenFilename(HWND hMainWindow, WCHAR *pszFilters, __out WCHAR *pszFilepath, DWORD dwBufSize, __out DWORD *pdwErrCode);

BOOL CALLBACK fGetProcIdDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK fGetNewProgramDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif // _GUIMANAGER_H
