
#ifndef _DEBUG_H
#define _DEBUG_H

#include "Common.h"
#include "UICreator.h"
#include "CHelpLibDll.h"
#include "MenuItems.h"

#define EXITCODE_TARGET_TERM    0xDEAD

typedef struct _DebugInfo {
    BOOL fDebuggingActiveProcess;
    BOOL fBreakAtMain;
    WCHAR szTargetPath[SLEN_MAXPATH];
    DWORD dwProcessID;
    HWND hMainWindow;
    HMENU hMainMenu;
    WCHAR szInitSyncEvtName[SLEN_EVENTNAMES];
    TABPAGEINFO stTabPageInfo;
}DEBUGINFO, *PDEBUGINFO;

DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs);


#endif // _DEBUG_H
