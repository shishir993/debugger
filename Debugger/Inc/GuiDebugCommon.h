#pragma once

#include "Common.h"

#define THTYPE_MAIN     0
#define THTYPE_WORKER   1

#define LV_THREAD_NUMITEMS  5

typedef int THTYPE;

static WCHAR aszThreadTypes[][SLEN_COMMON32] = { L"Main Thread", L"Worker Thread" };

typedef struct ListView_ThreadInfo {
    DWORD dwThreadId;
    DWORD dwEIPLocation;
    WCHAR szFunction[SLEN_COMMON64];
    THTYPE thType;
    INT iThreadPri;
}LV_THREADINFO, *PLV_THREADINFO;
