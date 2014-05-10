
#ifndef _UICREATOR_H
#define _UICREATOR_H

#include "Common.h"
#include "UICommon.h"

typedef struct _TabPageInfo {
    int iTabIndex;
    HWND hMainTab;
    HWND hEditDisass;
    HWND hTabBottom;
    HWND hListCallStack;
    HWND hListRegisters;
    HWND hListThreads;
    HWND hEditCommand;
    HWND hStaticCommand;

    HFONT hFixedFont;
}TABPAGEINFO, *PTABPAGEINFO;

BOOL fCreateMainTabControl(HWND hMainWnd, __out HWND *phTabControl, __out DWORD *pdwErrCode);
BOOL fCreateTabPage(HWND hTab, __out PTABPAGEINFO pstTabPageInfo, __out DWORD *pdwErrCode);
void vDeleteTabPage(HWND hTab, PTABPAGEINFO pstTabPageInfo);
BOOL fSetTabPageText(HWND hTab, int iTabIndex, PWCHAR pszTabText);

#endif // _UICREATOR_H
