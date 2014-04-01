
#include "Inc\Breakpoint.h"

#define BPTYPE_SINGLEHIT 	1 // for single stepping (step {into|out|over}), run to cursor
#define BPTYPE_USER		    2 // user specified

#define MAX_BREAKPOINTS     256 // maximum logical breakpoints

typedef int BPTYPE;    // specifies breakpoint type

typedef struct _ActualBreakpoint {
    BOOL fResolved;
    int nReferences;
    DWORD dwAddrInTarget;
    BYTE bOrigCodeByte;
}ACTUALBP, * PACTUALBP;


typedef struct _Breakpoint {
    BOOL fOccupied;
    WCHAR szExecutableName[SLEN_MAXPATH];
    WCHAR szSourceFileName[SLEN_MAXPATH];
    int iLineNum;
    BPTYPE bpType;
    PACTUALBP pstActualBp;
}BREAKPOINT, * PBREAKPOINT;

// ** Function Definitions **
BOOL fBpInitialize(__out void **ppBreakpoints)
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    // Reserve memory for list of breakpoints
    PBREAKPOINT pBpList = NULL;

    if(!fChlMmAlloc((void**)&pBpList, sizeof(BREAKPOINT) * MAX_BREAKPOINTS, &dwErrorCode))
    {
        SetLastError(dwErrorCode);
        return FALSE;
    }

    return TRUE;
}

BOOL fBpTerminate(void *pBreakpoints)
{

#if _DEBUG
            
#endif

}
