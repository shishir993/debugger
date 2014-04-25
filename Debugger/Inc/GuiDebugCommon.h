#pragma once

#include "Common.h"
#include "UICreator.h"

#define THTYPE_MAIN     0
#define THTYPE_WORKER   1

#define LV_REGS_NUMCOLUMNS    2
#define LV_THREAD_NUMCOLUMNS  5

#define FREEIF_GUIDBGCOMM(ptr)    if(ptr && ptr->fFreeThis) { vChlMmFree((void**)&ptr); }

typedef int THTYPE;

static WCHAR aszThreadTypes[][SLEN_COMMON32] = { L"Main", L"Worker" };
static WCHAR *apszRegNames[] = { L"EAX", L"EBX", L"ECX", L"EDX", L"ESI", L"EDI", L"ESP", L"EBP", L"EIP" };

typedef struct _GuiDbgComm {
    // Indicates whether the receiver must free this memory or not
    BOOL fFreeThis;

    // threadID which is sending/receiving the message
    DWORD dwThreadID;
    
    // tab index and handles to all tabitem children
    TABPAGEINFO stTabPageInfo;

    // Indicates whether Gui is sending the detach message due to debugger main window exit
    BOOL GD_fDetachOnDebuggerExit;

}GUIDBGCOMM, *PGUIDBGCOMM;

typedef struct ListView_ThreadInfo {
    DWORD dwThreadId;
    DWORD dwEIPLocation;
    WCHAR szFunction[SLEN_COMMON64];
    THTYPE thType;
    INT iThreadPri;
}LV_THREADINFO, *PLV_THREADINFO;
