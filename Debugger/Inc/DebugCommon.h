
#ifndef _DEBUGCOMMON_H
#define _DEBUGCOMMON_H

#include "Common.h"
#include "UICreator.h"
#include "CHelpLibDll.h"

#define DSTATE_INVALID          0
#define DSTATE_RUNNING          1
#define DSTATE_DEBUGGING        2
#define DSTATE_SINGLESTEPPING   3
#define DSTATE_MODBREAKPOINT    4   // handling state after breakpoint hit

// Action the user specified when an unexpected BP was encountered
#define DBG_CONT_ABORT  10
#define DBG_CONT_BREAK  11

#define MAX_BREAKPOINTS     256 // maximum logical breakpoints

typedef int BPTYPE;    // specifies breakpoint type

typedef struct _BreakpointInterface {
    int aiIdentifiers[MAX_BREAKPOINTS];
    PCHL_LLIST pstLinkedListBp;
}BPLIST, *PBPLIST;

// Structure used to pass BP info between caller and callee
typedef struct _BpInfo {
    int id;             // specify when removing a breakpoint
    BPTYPE iBpType;

    PWCHAR pszExecutableName;
    PWCHAR pszSourceFileName;
    int iLineNum;

    DWORD dwTargetAddr;

    HANDLE hTargetProcess;

}BPINFO, *PBPINFO;

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

typedef struct _TargetInfo {
    PDEBUGINFO pstDebugInfoFromGui;

    BOOL fCreateProcessEventRecvd;

    DWORD dwPID;
    DWORD dwMainThreadID;
    
    CREATE_PROCESS_DEBUG_INFO stProcessInfo;
 
    HANDLE hFileMapObj;
    HANDLE hFileMapView;

    PIMAGE_NT_HEADERS pstNtHeaders;
    DWORD dwCodeStart;
    DWORD dwCodeSize;
    DWORD dwCodeSecVirtAddr;

    CHL_HTABLE *phtDllsLoaded;
    CHL_HTABLE *phtThreads;
    PBPLIST pListBreakpoint;
    BPINFO stPrevBpInfo;
    
    int nCurThreads;
    int nTotalProcesses;
    int nTotalThreads;

    int nCurDllsLoaded;
    int nTotalDllsLoaded;

    int iDebugState;

    LPDEBUG_EVENT lpDebugEvent;
}TARGETINFO, *PTARGETINFO;

#endif // _DEBUGCOMMON_H
