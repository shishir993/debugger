
#ifndef _DEBUG_H
#define _DEBUG_H

#include "Common.h"
#include "UICreator.h"
#include "CHelpLibDll.h"

typedef struct _DebugInfo {
    BOOL fDebuggingActiveProcess;
    WCHAR *pszTargetPath;       // MUST FREE this. Heap variable.
    DWORD dwProcessID;
    HWND hMainWindow;
    WCHAR *pszInitSyncEvtName;  // DO NOT FREE this. Stack variable.
    TABPAGEINFO stTabPageInfo;
}DEBUGINFO, *PDEBUGINFO;

DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs);


#endif // _DEBUG_H
