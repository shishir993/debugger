
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

typedef struct _GuiDbgComm {
    DWORD dwThreadID;		    // threadID which is sending/receiving the message
    TABPAGEINFO stTabPageInfo;	// tab index and handles to all tabitem children
}GUIDBGCOMM, *PGUIDBGCOMM;


DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs);


#endif // _DEBUG_H
