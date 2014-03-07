
#include "Inc\UICreator.h"
#include "Inc\DebuggerDP.h"
#include "Inc\SourceHelpers.h"

extern HINSTANCE g_hMainInstance;

static BOOL fInsertTabItem(HWND hTab, WCHAR *pszText, __out int *piNewIndex, __out DWORD *pdwErrCode);

BOOL fCreateMainTabControl(HWND hMainWnd, __out HWND *phTabControl, DWORD *pdwErrCode)
{
    ASSERT(ISVALID_HANDLE(hMainWnd));
    ASSERT(phTabControl);
    ASSERT(pdwErrCode);
    
    RECT rcClientArea;
    HWND hTab = NULL;

    if(!GetClientRect(hMainWnd, &rcClientArea))
    {
        // todo: log error
        goto error_return;
    }

    hTab = CreateWindow(WC_TABCONTROL, NULL, WS_CHILD|WS_VISIBLE, rcClientArea.left + 5, rcClientArea.top + 5, 
                        rcClientArea.right - 10, rcClientArea.bottom - 10, hMainWnd, (HMENU)IDC_MAINTAB, g_hMainInstance, NULL);

    if(ISNULL(hTab))
    {
        // todo: log error
        goto error_return;
    }

    *phTabControl = hTab;
    return TRUE;
    
    error_return:
    *pdwErrCode = GetLastError();
    return FALSE;
}

BOOL fCreateTabPage(HWND hTab, __out PTABPAGEINFO pstTabPageInfo, __out DWORD *pdwErrCode)
{
    ASSERT(ISVALID_HANDLE(hTab));
    ASSERT(pstTabPageInfo);
    ASSERT(pdwErrCode);

    RECT rcTabWindow;
    RECT rcTabDisplay;

    HWND hDisass, hListCStack, hListRegisters, hListThreads, hTabBottom, hEditCommand, hStaticCommand;

    // Init all handles to NULL
    hDisass = hListCStack = hListRegisters = hListThreads = hTabBottom = hEditCommand = hStaticCommand = NULL;

    // Insert a new tab item
    if(!fInsertTabItem(hTab, L"Test", NULL, NULL))
    {
        goto error_return;
    }

    if(!GetWindowRect(hTab, &rcTabWindow))
    {
        goto error_return;
    }

    CopyRect(&rcTabDisplay, &rcTabWindow);
    SendMessage(hTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTabDisplay);
    
    ScreenToClient(hTab, (LPPOINT)&rcTabDisplay.left);
    ScreenToClient(hTab, (LPPOINT)&rcTabDisplay.right);

    // Bounding rectangle of the tab page's display area
    int x0 = rcTabDisplay.left;
    int y0 = rcTabDisplay.top;
    int xMax = rcTabDisplay.right;
    int yMax = rcTabDisplay.bottom;

    // For Disass window and bottom tab
    int x50 = int(xMax * 0.50);
    int y75 = int(yMax * 0.75);
    int leftCtrlWidth = x50 - x0 - PADDING_LONE - PADDING_TOGETHER;
    int disassHeight = y75 - y0 - PADDING_LONE - PADDING_TOGETHER;
    int bottomTabHeight = yMax - y75 - PADDING_LONE - PADDING_TOGETHER;

    // For the three list views and command window
    int xRightCtrl = x50 + PADDING_LONE + PADDING_TOGETHER;
    int y30 = int(yMax * 0.3 + PADDING_LONE + PADDING_TOGETHER);
    int y60 = y30 + y30 + PADDING_TOGETHER;
    int y90 = y60 + y30 + PADDING_TOGETHER;
    int rightCtrlWidth = xMax - x50 - PADDING_LONE - PADDING_TOGETHER;
    int commandHeight = yMax - y90 - PADDING_LONE * 3 - PADDING_TOGETHER;

    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the disass window
    hDisass = CreateWindow( 
                            WC_EDIT, 
                            NULL, 
                            WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_READONLY,
                            x0 + PADDING_LONE,
                            y0 + PADDING_LONE,
                            leftCtrlWidth,
                            disassHeight,
                            hTab,
                            (HMENU)IDC_EDIT_DISASS,
                            hInstance,
                            NULL);

    ISNULL_GOTO(hDisass, error_return);

    // Three list views
    hListCStack = CreateWindow(
                                WC_LISTVIEW,
                                NULL,
                                WS_CHILD | LVS_REPORT,
                                xRightCtrl,
                                y0 + PADDING_LONE,
                                rightCtrlWidth,
                                y30 - PADDING_LONE - PADDING_TOGETHER,
                                hTab,
                                (HMENU)IDC_LIST_CALLSTACK,
                                hInstance,
                                NULL);

    ISNULL_GOTO(hListCStack, error_return);

    hListRegisters = CreateWindow( 
                                    WC_LISTVIEW,
                                    NULL,
                                    WS_CHILD | LVS_REPORT,
                                    xRightCtrl,
                                    y30 + PADDING_LONE + PADDING_TOGETHER,
                                    rightCtrlWidth,
                                    y30 - PADDING_LONE - PADDING_TOGETHER,
                                    hTab,
                                    (HMENU)IDC_LIST_REGISTERS,
                                    hInstance,
                                    NULL);

    ISNULL_GOTO(hListRegisters, error_return);

    hListThreads = CreateWindow( 
                                    WC_LISTVIEW,
                                    NULL,
                                    WS_CHILD | LVS_REPORT,
                                    xRightCtrl,
                                    y60 + PADDING_LONE*2 + PADDING_TOGETHER,
                                    rightCtrlWidth,
                                    y30 -PADDING_LONE*2 - PADDING_TOGETHER,
                                    hTab,
                                    (HMENU)IDC_LIST_THREADS,
                                    hInstance,
                                    NULL);

    ISNULL_GOTO(hListThreads, error_return);

    // Command static and edit controls
    hStaticCommand = CreateWindow(
                                    WC_STATIC,
                                    NULL,
                                    WS_CHILD | WS_VISIBLE,
                                    xRightCtrl,
                                    y90 + PADDING_LONE * 3 + PADDING_TOGETHER,
                                    50,
                                    commandHeight,
                                    hTab,
                                    NULL,
                                    hInstance,
                                    NULL);

    ISNULL_GOTO(hStaticCommand, error_return);
    BOOL ret = SendMessage(hStaticCommand, WM_SETTEXT, 0, (LPARAM)L"Command: ");

    RECT rcStatic;
    GetWindowRect(hStaticCommand, &rcStatic);
    ScreenToClient(hStaticCommand, (LPPOINT)&rcStatic.left);
    ScreenToClient(hStaticCommand, (LPPOINT)&rcStatic.right);
    hEditCommand = CreateWindow(
                                WC_EDIT,
                                NULL,
                                WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT,
                                xRightCtrl + (rcStatic.right - rcStatic.left) + PADDING_TOGETHER,
                                y90 + PADDING_LONE * 3 + PADDING_TOGETHER,
                                rightCtrlWidth - rcStatic.right - rcStatic.left - PADDING_TOGETHER,
                                commandHeight,
                                hTab,
                                NULL,
                                hInstance,
                                NULL);

    ISNULL_GOTO(hEditCommand, error_return);

    // The bottom tab control
    hTabBottom = CreateWindow(
                                WC_TABCONTROL,
                                NULL,
                                WS_CHILD | WS_VISIBLE,
                                x0 + PADDING_LONE,
                                y75 + PADDING_LONE + PADDING_TOGETHER,
                                leftCtrlWidth,
                                bottomTabHeight,
                                hTab,
                                (HMENU)IDC_TAB_BOTTOM,
                                hInstance,
                                NULL);
    ISNULL_GOTO(hTabBottom, error_return);

    return TRUE;

    error_return:
    SET_ERRORCODE_PTR(pdwErrCode);
    ZeroMemory(pstTabPageInfo, sizeof(TABPAGEINFO));
    return FALSE;
}

BOOL fInsertTabItem(HWND hTab, WCHAR *pszText, __out int *piNewIndex, __out DWORD *pdwErrCode)
{
    ASSERT(hTab);

    int nTabs;
    int iNewIndex;
    TCITEM tcItem;

    // First, get the number of items(pages) already in tab
    nTabs = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);

    // Setup the TCITEM structure
    ZeroMemory(&tcItem, sizeof(TCITEM));

    if(!ISNULL(pszText))
    {
        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = pszText;
    }

    // Insert the item
    iNewIndex = SendMessage(hTab, TCM_INSERTITEM, nTabs + 1, (LPARAM)&tcItem);
    if(iNewIndex == -1)
    {
        SET_ERRORCODE_PTR(pdwErrCode);
        return FALSE;
    }

    if(piNewIndex)
    {
        *piNewIndex = iNewIndex;
    }
    return TRUE;
}