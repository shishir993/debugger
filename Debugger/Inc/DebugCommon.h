
#ifndef _DEBUGCOMMON_H
#define _DEBUGCOMMON_H

#include "Common.h"
#include "UICreator.h"
#include "Defines.h"

#define DSTATE_INVALID                  0

// Target process is running and is not stopped by the debugger
#define DSTATE_RUNNING                  1

// DO NOT process debug event loop, just wait for user input
// and take action accordingly
#define DSTATE_DEBUGGING                2

// User is single stepping(step into)
// Before: we are waiting for user input, DO NOT process debug event loop
// After: user has pressed step into, process debug event loop(expect a SS exception)
#define DSTATE_SINGLESTEP_BEFORE        3
#define DSTATE_SINGLESTEP_AFTER         4      

// Debugger is using single step for its own purpose
#define DSTATE_SINGLESTEP_DBG           5

#define DSTATE_WAITFOR_DBGBREAK         6

// Set by the Gui message loop handler or the debug event loop handler
// indicating that the debug thread must exit now. This will be used 
// when the target process exits(either by itself or we killed it) or
// we detach from it.
#define DSTATE_EXIT                     7

// State after breakpoint hit. We must wait for user input and DO NOT
// let target process continue execution when in this state
#define DSTATE_BREAKPOINTWAIT           DSTATE_DEBUGGING

#define DSTATE_START    DSTATE_INVALID
#define DSTATE_END      DSTATE_EXIT

// Action the user specified when an unexpected BP was encountered
#define DBG_CONTCUSTOM_ABORT  10
#define DBG_CONTCUSTOM_BREAK  11

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

typedef struct _PrevBpInfo {
    // Breakpoint hit in which target thread
    DWORD dwThreadId;

    // Info about the breakpoint that was hit
    BPINFO stBpInfo;
}PREVBPINFO;

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
    PREVBPINFO stPrevBpInfo;

    // To store info during single stepping (using TF)
    DWORD dwSSThreadId;
    DWORD dwSSTargetAddr;
    
    int nCurThreads;
    int nTotalProcesses;
    int nTotalThreads;

    int nCurDllsLoaded;
    int nTotalDllsLoaded;

    int iDebugState;
    int iPrevDebugState;

    BOOL fDetachOnDebuggerExit;

    LPDEBUG_EVENT lpDebugEvent;
}TARGETINFO, *PTARGETINFO;

#endif // _DEBUGCOMMON_H
