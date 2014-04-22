
#ifndef _BREAKPOINTS_H
#define _BREAKPOINTS_H

#include "Common.h"
#include "CHelpLibDll.h"
#include "StringLengths.h"
#include "CHelpLibDll.h"
#include "DebugCommon.h"

#define BPTYPE_USERSINGLEHIT 	    0x0001    // for single stepping (step {out|over}), run to cursor
#define BPTYPE_USERMULTIHIT         0x0002    // user specified
#define BPTYPE_DEBUGGERSINGLEHIT    0x0004
#define BPTYPE_DEBUGGERMULTIHIT     0x0008

#define BPTYPE_ASMLEVEL     0x0010    // breakpoint at a specific address target memory
#define BPTYPE_SOURCELEVEL  0x0020    // breakpoint at a line number in a source file related to a specific exe/dll

typedef struct _ActualBreakpoint {
    BOOL fResolved;
    DWORD dwAddrInTarget;
    BYTE bOrigCodeByte;
}ACTUALBP, *PACTUALBP;

// The below structure is the one stored as the linked list node
typedef struct _Breakpoint {
    int id;
    int nReferences;
    
    BPTYPE bpType;

    int iLineNum;
    WCHAR szExecutableName[SLEN_MAXPATH];
    WCHAR szSourceFileName[SLEN_MAXPATH];
    
    ACTUALBP stActualBp;
}BREAKPOINT, *PBREAKPOINT;

// ** Functions **
BOOL fBpInitialize(__out PBPLIST *ppBreakpoints);
BOOL fBpTerminate(PBPLIST pBreakpoints);

BOOL fBpInsert(PBPLIST pstBpList, PBPINFO pstBpInfo, PTARGETINFO pstTargetInfo, __out OPTIONAL PINT piBpID);
BOOL fBpRemove(PBPLIST pstBpList, PBPINFO pstBpInfo, PINT piBpID, PTARGETINFO pstTargetInfo);
BOOL fBpFind(PBPLIST pstBpList, __inout PBPINFO pstBpInfo, PINT piBpID);

#endif // _BREAKPOINTS_H
