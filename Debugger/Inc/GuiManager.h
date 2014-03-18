
#ifndef _GUIMANAGER_H
#define _GUIMANAGER_H

#include "Common.h"
#include "UICommon.h"
#include "CHelpLibDll.h"

BOOL fGuiInitialize(__out DWORD *pdwErrCode);
BOOL fGuiAddTab(int tabIndex, DWORD threadId, __out DWORD *pdwErrCode);
BOOL fGuiRemTab(int tabIndex, __out DWORD *pdwErrCode);
BOOL fGuiFindTab(int tabIndex, __out DWORD *pdwThreadId, __out DWORD *pdwErrCode);

BOOL fGuiGetOpenFilename(HWND hMainWindow, WCHAR *pszFilters, __out WCHAR **ppszFilepath, __out DWORD *pdwErrCode);

#endif // _GUIMANAGER_H
