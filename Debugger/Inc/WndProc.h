
#ifndef _WNDPROC_H
#define _WNDPROC_H

#include "Common.h"

#include <io.h>
#include <fcntl.h>

#include "UICommon.h"
#include "Logger.h"
#include "UICreator.h"
#include "CHelpLibDll.h"

typedef struct _DbgSessionStart {
    BOOL fDebuggingActiveProcess;
    BOOL fBreakAtMain;
    DWORD dwTargetPID;
    WCHAR szTargetPath[SLEN_MAXPATH];
}DBG_SESSIONSTART, *PDBG_SESSIONSTART;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif // _WNDPROC_H
