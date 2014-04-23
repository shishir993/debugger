
#ifndef _DEBUGTHREADHELPERS_H
#define _DEBUGTHREADHELPERS_H

#include "Common.h"
#include "CHelpLibDll.h"
#include "DebugCommon.h"

BOOL fGetExceptionName(DWORD excode, __out WCHAR *pwsBuffer, int bufSize);

BOOL fAddThread(CHL_HTABLE *phtThreads, DWORD dwThreadId, LPCREATE_THREAD_DEBUG_INFO lpThreadInfo);
BOOL fRemoveThread(CHL_HTABLE *phtThreads, DWORD dwThreadId, __out LPCREATE_THREAD_DEBUG_INFO lpThreadInfo);
BOOL fGetThreadHandle(CHL_HTABLE *phtThreads, DWORD id, __out HANDLE *phThread);
BOOL fDeleteThreadsTable(CHL_HTABLE *phtThreads);
BOOL fSuspendAllThreads(CHL_HTABLE *phtThreads);
BOOL fResumeAllThreads(CHL_HTABLE *phtThreads);
BOOL fIsNtDllLoaded(CHL_HTABLE *phtDllTable, __out DWORD *pdwBaseAddress);

BOOL fBreakAtEntryPoint(PTARGETINFO pstTargetInfo);
BOOL fHandleExceptionBreakpoint(PTARGETINFO pstTargetInfo, __out PDWORD pdwContinueStatus);

void vSetContinueStatusFromUser(DWORD dwExceptionCode, DWORD dwExceptionAddress, BOOL fFirstChance, PDWORD pdwContinueStatus);

BOOL fDecrementInstPointer(CHL_HTABLE *phtThreads, DWORD dwThreadId);

#endif // _DEBUGTHREADHELPERS_H
