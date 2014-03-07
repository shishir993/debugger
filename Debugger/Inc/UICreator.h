
#ifndef _UICREATOR_H
#define _UICREATOR_H

#include "Common.h"
#include "UICommon.h"

typedef struct _TabPageInfo {
    int iTabIndex;
    HWND hEditDisass;
    HWND hTabBottom;
    HWND hListCallStack;
    HWND hListRegisters;
    HWND hListThreads;
    HWND hEditCommand;
    HWND hStaticCommand;
}TABPAGEINFO, *PTABPAGEINFO;

BOOL fCreateMainTabControl(HWND hMainWnd, __out HWND *phTabControl, __out DWORD *pdwErrCode);
BOOL fCreateTabPage(HWND hTab, __out PTABPAGEINFO pstTabPageInfo, __out DWORD *pdwErrCode);

#endif // _UICREATOR_H