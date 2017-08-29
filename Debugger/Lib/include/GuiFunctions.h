
// GuiFunctions.h
// Contains functions that provide IO operation services
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      03/25/14 Initial version
//      09/09/14 Refactor to store defs in individual headers.
//      09/12/14 Naming convention modifications
//

#ifndef _GUIFUNCTIONS_H
#define _GUIFUNCTIONS_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"
#include "MemFunctions.h"

// -------------------------------------------
// Functions exported

DllExpImp HRESULT CHL_GuiCenterWindow(_In_ HWND hWnd);

// Given the window handle and the number of characters, returns the 
// width and height in pixels that will be occupied by a string of that
// consisting of those number of characters
DllExpImp HRESULT CHL_GuiGetTextArea(_In_ HWND hWindow, _In_ int nCharsInText, _Out_ int *pnPixelsWidth, _Out_ int *pnPixelsHeight);

DllExpImp HRESULT CHL_GuiInitListViewColumns(
    _In_ HWND hList, 
    _In_ WCHAR *apszColumNames[], 
    _In_ int nColumns, 
    _In_opt_ int *paiColumnSizePercent);

DllExpImp HRESULT CHL_GuiAddListViewRow(
    _In_ HWND hList, 
    _In_ WCHAR *apszItemText[], 
    _In_ int nItems, 
    _In_opt_ LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // _GUIFUNCTIONS_H
