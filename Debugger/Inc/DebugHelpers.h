
#ifndef _DEBUGTHREADHELPERS_H
#define _DEBUGTHREADHELPERS_H

#include "Common.h"
#include "Defines.h"
#include "DebugCommon.h"
#include "GuiDebugCommon.h"

// Bit number of the TrapFlag in EFlags register
#define BITPOS_EFLAGS_TF    8

BOOL fGetExceptionName(DWORD excode, __out WCHAR *pwsBuffer, int bufSize);

BOOL fAddThread(CHL_HTABLE *phtThreads, DWORD dwThreadId, LPCREATE_THREAD_DEBUG_INFO lpThreadInfo);
BOOL fRemoveThread(CHL_HTABLE *phtThreads, DWORD dwThreadId, __out LPCREATE_THREAD_DEBUG_INFO lpThreadInfo);
BOOL fGetThreadHandle(CHL_HTABLE *phtThreads, DWORD id, __out HANDLE *phThread);
BOOL fDeleteThreadsTable(CHL_HTABLE *phtThreads);
BOOL fSuspendAllThreads(CHL_HTABLE *phtThreads);
BOOL fResumeAllThreads(CHL_HTABLE *phtThreads);
BOOL fIsNtDllLoaded(CHL_HTABLE *phtDllTable, __out DWORD *pdwBaseAddress);

// Breakpoint functions
BOOL fBreakAtEntryPoint(PTARGETINFO pstTargetInfo);
BOOL fHandleExceptionBreakpoint(PTARGETINFO pstTargetInfo, __out PDWORD pdwContinueStatus);
BOOL fReInsertBPIf(PTARGETINFO pstTargetInfo, PREVBPINFO *pstBpInfo);
BOOL fDecrementInstPointer(CHL_HTABLE *phtThreads, DWORD dwThreadId);
BOOL fSetTrapFlag(CHL_HTABLE *phtThreads, DWORD dwThreadId);
BOOL fClearTrapFlag(CHL_HTABLE *phtThreads, DWORD dwThreadId);

void vSetContinueStatusFromUser(DWORD dwExceptionCode, DWORD dwExceptionAddress, BOOL fFirstChance, PDWORD pdwContinueStatus);

// Debugger state changes
void vSetMenuItemsState(PTARGETINFO pstTargetInfo);
void vDebuggerStateChange(PTARGETINFO pstTargetInfo, int iNewState);

// Gui management
BOOL fUpdateThreadsListView(HWND hList, CHL_HTABLE *phtThreads, HANDLE hMainThread);
BOOL fUpdateRegistersListView(HWND hList, DWORD dwThreadId);
BOOL fShowDisassembly(PTARGETINFO pstTargetInfo, DWORD dwStartFromTargetAddress);

#endif // _DEBUGTHREADHELPERS_H
