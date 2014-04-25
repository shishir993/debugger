
#ifndef _GUIMANAGER_H
#define _GUIMANAGER_H

#include "Common.h"
#include <Psapi.h>

#include "UICommon.h"
#include "CHelpLibDll.h"
#include "GuiDebugCommon.h"

BOOL fGuiInitialize(__out DWORD *pdwErrCode);
void vGuiExit();
BOOL fGuiAddTab(int tabIndex, DWORD threadId, __out DWORD *pdwErrCode);
BOOL fGuiRemTab(int tabIndex, __out DWORD *pdwErrCode);
BOOL fGuiFindTab(int tabIndex, __out DWORD *pdwThreadId, __out DWORD *pdwErrCode);
BOOL fOnExitDetachTargets();

BOOL fGuiGetOpenFilename(HWND hMainWindow, WCHAR *pszFilters, __out WCHAR *pszFilepath, DWORD dwBufSize, __out DWORD *pdwErrCode);

BOOL CALLBACK fGetProcIdDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK fGetNewProgramDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL fGuiUpdateThreadsList(HWND hThreadListView, PLV_THREADINFO pstLvThreadInfo, int nItems);
BOOL fGuiUpdateRegistersList(HWND hThreadListView, WCHAR *apszNames[], DWORD *padwValues, int nItems);

#endif // _GUIMANAGER_H
