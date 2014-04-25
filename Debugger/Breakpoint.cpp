
#include "Inc\Breakpoint.h"

extern PLOGGER pstLogger;

#define OPCODE_BREAKPOINT   0xCC

static int iGetFirstFreeIdentifier(PINT paiIdentifiers);
static BOOL fBpInsert_Internal(HANDLE hTargetProcess, DWORD dwTargetAddr, __out PBYTE pbOrigCodeByte);
static BOOL fBpRemove_Internal(HANDLE hTargetProcess, DWORD dwTargetAddr, BYTE bOrigCodeByte);
static BOOL fBpCompare(PVOID pvLeft, PVOID pvRight);

// ** Function Definitions **
BOOL fBpInitialize(__out PBPLIST *ppstBpList)
{
    ASSERT(ppstBpList);

    DWORD dwErrorCode = ERROR_SUCCESS;
    PBPLIST pstBpList = NULL;

    if(!fChlMmAlloc((void**)&pstBpList, sizeof(BPLIST), &dwErrorCode))
    {
        logerror(pstLogger, L"%s(): fChlMmAlloc() failed %u", __FUNCTIONW__, GetLastError());
        return FALSE;
    }

    // pstBpList->aiIdentifiers[] will be zero filled

    if(!fChlDsCreateLL(&pstBpList->pstLinkedListBp, LL_VAL_PTR, MAX_BREAKPOINTS))
    {
        logerror(pstLogger, L"%s(): fChlDsCreateLL() failed %u", __FUNCTIONW__, GetLastError());
        vChlMmFree((void**)&pstBpList);
        return FALSE;
    }

    *ppstBpList = pstBpList;
    return TRUE;
}

BOOL fBpTerminate(PBPLIST pBreakpoints)
{
    ASSERT(pBreakpoints);
    ASSERT(pBreakpoints->pstLinkedListBp);

    BOOL fRetVal = fChlDsDestroyLL(pBreakpoints->pstLinkedListBp);

    vChlMmFree((void**)&pBreakpoints);

    return fRetVal;
}

BOOL fBpInsert(PBPLIST pstBpList, PBPINFO pstBpInfo, PTARGETINFO pstTargetInfo, __out OPTIONAL PINT piBpID)
{
    ASSERT(pstBpList);
    ASSERT(pstBpInfo);
    ASSERT(pstTargetInfo);

    BREAKPOINT stBreakpoint;
    int id;

    id = iGetFirstFreeIdentifier(pstBpList->aiIdentifiers);

    // Assume 1-1 mapping between logical and actual breakpoint for now
    // TODO: implement n-1 mapping

    stBreakpoint.id = id;
    stBreakpoint.bpType = pstBpInfo->iBpType;   
     
    if(pstBpInfo->iBpType & BPTYPE_ASMLEVEL)
    {
        stBreakpoint.stActualBp.dwAddrInTarget = pstBpInfo->dwTargetAddr;
    }
    else
    {
        // TODO: support this
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto error_return;
    }

    stBreakpoint.nReferences = 1;

    // Insert breakpoint into target process
    if(!fBpInsert_Internal(
            pstTargetInfo->stProcessInfo.hProcess, 
            pstBpInfo->dwTargetAddr, 
            &stBreakpoint.stActualBp.bOrigCodeByte))
    {
        goto error_return;
    }

    stBreakpoint.stActualBp.fResolved = TRUE;

    // Insert into list of breakpoints
    if(!fChlDsInsertLL(pstBpList->pstLinkedListBp, &stBreakpoint, sizeof(stBreakpoint)))
    {
        logerror(pstLogger, L"%s(): fChlDsInsertLL() failed %u", __FUNCTIONW__, GetLastError());

        // Remove the inserted breakpoint
        if(stBreakpoint.nReferences == 1)
        {
            fBpRemove_Internal(pstTargetInfo->stProcessInfo.hProcess, pstBpInfo->dwTargetAddr, stBreakpoint.stActualBp.bOrigCodeByte);
        }

        goto error_return;
    }

    IFPTR_SETVAL(piBpID, id);
    return TRUE;

error_return:
    return FALSE;
}

BOOL fBpRemove(PBPLIST pstBpList, PBPINFO pstBpInfo, PTARGETINFO pstTargetInfo)
{
    ASSERT(pstBpList);
    ASSERT(pstBpInfo);

    // Assume 1-1 mapping between logical and actual breakpoint for now
    // TODO: implement n-1 mapping

    // Only ASM level breakpoints now
    // TODO: SOURCE level and using the identifier

    BREAKPOINT stBpToFind;
    PBREAKPOINT pstBpFound = NULL;

    // TODO: assign bpType as well
    stBpToFind.stActualBp.dwAddrInTarget = pstBpInfo->dwTargetAddr;

    // Remove from the linked list
    if(!fChlDsRemoveLL(pstBpList->pstLinkedListBp, &stBpToFind, TRUE, fBpCompare, (void**)&pstBpFound))
    {
        logerror(pstLogger, L"%s(): fChlDsRemoveLL() failed %u", __FUNCTIONW__, GetLastError());
        goto error_return;
    }

    // TODO: remove from linked list only after successful removal of actual BP?

    // Replace breakpoint instruction with original opcode
    if(pstBpFound->nReferences == 1)
    {
        dbgwprintf(L"%s(): Removing actual breakpoint at 0x%08x because #references == 1\n", __FUNCTIONW__, pstBpFound->stActualBp.dwAddrInTarget);
        if(!fBpRemove_Internal(pstTargetInfo->stProcessInfo.hProcess, pstBpFound->stActualBp.dwAddrInTarget, pstBpFound->stActualBp.bOrigCodeByte))
        {
            logerror(pstLogger, L"%s(): fBpRemove_Internal() failed %u", __FUNCTIONW__, GetLastError());
            goto error_return;
        }
    }

    vChlMmFree((void**)&pstBpFound);
    return TRUE;

error_return:
    IFPTR_FREE(pstBpFound);
    return FALSE;
}

BOOL fBpFind(PBPLIST pstBpList, __inout PBPINFO pstBpInfo, PINT piBpID)
{
    ASSERT(pstBpList);
    ASSERT(pstBpInfo);

    DBG_UNREFERENCED_PARAMETER(piBpID);

    BREAKPOINT stBpToFind;
    PBREAKPOINT pstBpFound = NULL;

    // TODO: suppoprt usage of piBpID also

    // TODO: assign bpType as well
    stBpToFind.stActualBp.dwAddrInTarget = pstBpInfo->dwTargetAddr;

    // Find in linked list
    if(!fChlDsFindLL(pstBpList->pstLinkedListBp, &stBpToFind, fBpCompare, (void**)&pstBpFound))
    {
        logerror(pstLogger, L"%s(): fChlDsFindLL() failed %u", __FUNCTIONW__, GetLastError());
        return FALSE;
    }

    // Populate out variable and return TRUE
    pstBpInfo->iBpType = pstBpFound->bpType;
    pstBpInfo->id = pstBpFound->id;
    pstBpInfo->dwTargetAddr = pstBpFound->stActualBp.dwAddrInTarget;

    return TRUE;
}

static int iGetFirstFreeIdentifier(PINT paiIdentifiers)
{
    ASSERT(paiIdentifiers);

    for(int index = 0; index < MAX_BREAKPOINTS; ++index)
    {
        if(paiIdentifiers[index] == 0)
        {
            return index + 1;
        }
    }

    logwarn(pstLogger, L"%s(): No free identifiers!", __FUNCTIONW__);
    return -1;
}

static BOOL fBpInsert_Internal(HANDLE hTargetProcess, DWORD dwTargetAddr, __out PBYTE pbOrigCodeByte)
{
    ASSERT(ISVALID_HANDLE(hTargetProcess));
    ASSERT(dwTargetAddr > 0);
    ASSERT(pbOrigCodeByte);

    BYTE bOrigCode;
    SIZE_T bytesReadWritten;

    BYTE bBreakpointInst = OPCODE_BREAKPOINT;

    // Get the original code byte first
    if(!ReadProcessMemory(hTargetProcess, (LPCVOID)dwTargetAddr, &bOrigCode, sizeof(bOrigCode), &bytesReadWritten))
    {
        return FALSE;
    }

    // TODO: check bytesReadWritten?

    dbgwprintf(L"Original code byte at 0x%08x: 0x%02x\n", dwTargetAddr, bOrigCode);

    // Overwrite with breakpoint instruction
    if(!WriteProcessMemory(hTargetProcess, (LPVOID)dwTargetAddr, &bBreakpointInst, sizeof(BYTE), &bytesReadWritten))
    {
        return FALSE;
    }

    // TODO: check bytesReadWritten?

    *pbOrigCodeByte = bOrigCode;
    return TRUE;
}

static BOOL fBpRemove_Internal(HANDLE hTargetProcess, DWORD dwTargetAddr, BYTE bOrigCodeByte)
{
    ASSERT(ISVALID_HANDLE(hTargetProcess));
    ASSERT(dwTargetAddr > 0);

#ifdef _DEBUG
    
    // Read the original code byte and assert that it is a 0x03 value
    BYTE bOrigCode;
    SIZE_T bytesReadWritten;

    // Get the original code byte first
    if(!ReadProcessMemory(hTargetProcess, (LPCVOID)dwTargetAddr, &bOrigCode, sizeof(bOrigCode), &bytesReadWritten))
    {
        logwarn(pstLogger, L"%s(): ReadProcessMemory() failed %u", __FUNCTIONW__, GetLastError());
    }
    else
    {
        wprintf(L"%s(): Opcode before replacement at 0x%08x: 0x%02x\n", __FUNCTIONW__, dwTargetAddr, bOrigCode);
        ASSERT(bOrigCode == OPCODE_BREAKPOINT);
    }

#endif

    if(!WriteProcessMemory(hTargetProcess, (LPVOID)dwTargetAddr, &bOrigCodeByte, sizeof(BYTE), &bytesReadWritten))
    {
        return FALSE;
    }

    // TODO: check bytesReadWritten?

    return TRUE;

}

static BOOL fBpCompare(PVOID pvLeft, PVOID pvRight)
{
    ASSERT(pvLeft);
    ASSERT(pvRight);

    PBREAKPOINT pstBpLeft = (PBREAKPOINT)pvLeft;
    PBREAKPOINT pstBpRight = (PBREAKPOINT)pvRight;

    // Only ASM level comparison now
    // TODO: SOURCE level comparison and using the identifier

    return pstBpLeft->stActualBp.dwAddrInTarget == pstBpRight->stActualBp.dwAddrInTarget;
}
