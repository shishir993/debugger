
#ifndef _DEBUG_H
#define _DEBUG_H

#include "Common.h"
#include "UICreator.h"
#include "CHelpLibDll.h"

typedef struct _DebugInfo {
    BOOL fDebuggingActiveProcess;
    WCHAR szTargetPath[SLEN_MAXPATH];
    DWORD dwProcessID;
    HWND hMainWindow;
    WCHAR szInitSyncEvtName[SLEN_EVENTNAMES];
    TABPAGEINFO stTabPageInfo;
}DEBUGINFO, *PDEBUGINFO;

DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs);


#endif // _DEBUG_H
