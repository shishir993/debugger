
#include "Inc\UICreator.h"
#include "Inc\DebuggerDP.h"
#include "Inc\SourceHelpers.h"
#include "CHelpLibDll.h"

extern HINSTANCE g_hMainInstance;
extern PLOGGER pstLogger;

static int aiColumnSizePercent_Threads[] = { 12, 12, 52, 12, 12 };
static int aiColumnSizePercent_Regs[] = { 15, 85 };

static WCHAR *aszColumnNames_Threads[] = { L"ThreadId", L"EIPLocation", L"Function", L"Type", L"Priority" };
static WCHAR *aszColumnNames_Regs[] = { L"Name", L"Value" };

struct _FontSelection {
    LOGFONT stLogFont;
    TEXTMETRIC stTextMetric;
};

// File local functions
static BOOL fInsertTabItem(HWND hTab, WCHAR *pszText, __out int *piNewIndex, __out DWORD *pdwErrCode);
BOOL fGetFixedWidthFont(HWND hWindow, HFONT *phFont);
static int CALLBACK EnumFontCallback(
    const LOGFONT *lpelfe,
    const TEXTMETRIC *lpntme,
    DWORD FontType,
    LPARAM lParam);

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

    hTab = CreateWindow(WC_TABCONTROL, NULL, WS_CHILD|WS_VISIBLE|WS_BORDER, rcClientArea.left + 5, rcClientArea.top + 5, 
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

    int iNewIndex = -1;
    HWND hEditDisass, hListCStack, hListRegisters, hListThreads, hTabBottom, hEditCommand, hStaticCommand;

    HFONT hFixedFont = NULL;

    // Init all handles to NULL
    hEditDisass = hListCStack = hListRegisters = hListThreads = hTabBottom = hEditCommand = hStaticCommand = NULL;

    // Insert a new tab item
    if(!fInsertTabItem(hTab, L"Test", &iNewIndex, NULL))
    {
        // todo: log
        goto error_return;
    }
    ASSERT(iNewIndex > -1);

    if(!GetWindowRect(hTab, &rcTabWindow))
    {
        // todo: log
        goto error_return;
    }

    CopyRect(&rcTabDisplay, &rcTabWindow);
    SendMessage(hTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTabDisplay);
    
    ScreenToClient(hTab, (LPPOINT)&rcTabDisplay.left);
    ScreenToClient(hTab, (LPPOINT)&rcTabDisplay.right);

    // Calculate width for the text static 'Command:' (without quotes) and the edit control
    WCHAR szTextCommand[] = L"Command:";
    int iTextWidth, iTextHeight;
    
    if(!fChlGuiGetTextArea(hTab, wcslen(szTextCommand), &iTextWidth, &iTextHeight))
    {
        logerror(pstLogger, L"%s(): fChlGuiGetTextArea() failed %u", GetLastError());
        goto error_return;
    }

    int iCursorHeight = GetSystemMetrics(SM_CYCURSOR);

    // Bounding rectangle of the tab page's display area
    int x0 = rcTabDisplay.left;
    int y0 = rcTabDisplay.top;
    int xMax = rcTabDisplay.right;
    int yMax = rcTabDisplay.bottom;

    int tabWidth = xMax - x0;
    int tabHeight = yMax - y0;

    int liHeight = (tabHeight - iCursorHeight) * 0.33;

    int w50 = tabWidth / 2;             // width 1/2
    int h75 = tabHeight * 0.75;         // height 3/4
    int h25 = tabHeight - h75;          // height 1/4

    // Get a fixed width font for use in the child controls
    if(!fGetFixedWidthFont(hTab, &hFixedFont))
    {
        goto error_return;
    }

    // Create the disass window
    hEditDisass = CreateWindow( 
                            WC_EDIT, 
                            NULL, 
                            WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_READONLY,
                            x0,
                            y0,
                            w50,
                            h75,
                            hTab,
                            (HMENU)IDC_EDIT_DISASS,
                            g_hMainInstance,
                            NULL);

    ISNULL_GOTO(hEditDisass, error_return);
    SendMessage(hEditDisass, WM_SETFONT, (WPARAM)hFixedFont, FALSE);

    RECT rcTemp;

    // Three list views
    hListCStack = CreateWindow(
                                WC_LISTVIEW,
                                NULL,
                                WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_ALIGNLEFT,
                                x0 + w50,
                                y0,
                                w50,
                                liHeight,
                                hTab,
                                (HMENU)IDC_LIST_CALLSTACK,
                                g_hMainInstance,
                                NULL);

    ISNULL_GOTO(hListCStack, error_return);
    GetWindowRect(hListCStack, &rcTemp);

    // TODO: initialize columns

    hListRegisters = CreateWindow( 
                                    WC_LISTVIEW,
                                    NULL,
                                    WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_ALIGNLEFT,
                                    x0 + w50,
                                    y0 + liHeight,
                                    w50,
                                    liHeight,
                                    hTab,
                                    (HMENU)IDC_LIST_REGISTERS,
                                    g_hMainInstance,
                                    NULL);

    ISNULL_GOTO(hListRegisters, error_return);

    SendMessage(hListRegisters, WM_SETFONT, (WPARAM)hFixedFont, FALSE);
    GetWindowRect(hListRegisters, &rcTemp);

    // Initialize columns
    if(!fChlGuiInitListViewColumns(hListRegisters, aszColumnNames_Regs, NELEMS_ARRAY(aszColumnNames_Regs), aiColumnSizePercent_Regs))
    {
        // todo: log error
        goto error_return;
    }

    hListThreads = CreateWindow( 
                                    WC_LISTVIEW,
                                    NULL,
                                    WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_ALIGNLEFT,
                                    x0 + w50,
                                    y0 + liHeight + liHeight,
                                    w50,
                                    liHeight,
                                    hTab,
                                    (HMENU)IDC_LIST_THREADS,
                                    g_hMainInstance,
                                    NULL);

    ISNULL_GOTO(hListThreads, error_return);
    SendMessage(hListThreads, WM_SETFONT, (WPARAM)hFixedFont, FALSE);

    GetWindowRect(hListThreads, &rcTemp);

    // Initialize columns
    if(!fChlGuiInitListViewColumns(hListThreads, aszColumnNames_Threads, NELEMS_ARRAY(aszColumnNames_Threads), aiColumnSizePercent_Threads))
    {
        // todo: log error
        goto error_return;
    }

    // Command static and edit controls
    hStaticCommand = CreateWindow(
                                    WC_STATIC,
                                    NULL,
                                    WS_CHILD | WS_BORDER | WS_VISIBLE,
                                    x0 + w50,
                                    yMax - iCursorHeight,
                                    iTextWidth,
                                    iCursorHeight,
                                    hTab,
                                    NULL,
                                    g_hMainInstance,
                                    NULL);

    ISNULL_GOTO(hStaticCommand, error_return);
    SendMessage(hStaticCommand, WM_SETTEXT, 0, (LPARAM)szTextCommand);

    GetWindowRect(hStaticCommand, &rcTemp);
    hEditCommand = CreateWindow(
                                WC_EDIT,
                                NULL,
                                WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT,
                                x0 + w50 + iTextWidth + 2,
                                yMax - iCursorHeight,
                                w50 - iTextWidth - 2,
                                iCursorHeight,
                                hTab,
                                NULL,
                                g_hMainInstance,
                                NULL);

    ISNULL_GOTO(hEditCommand, error_return);

    // The bottom tab control
    hTabBottom = CreateWindow(
                                WC_TABCONTROL,
                                NULL,
                                WS_CHILD | WS_BORDER | WS_VISIBLE,
                                x0,
                                y0 + h75,
                                w50,
                                h25,
                                hTab,
                                (HMENU)IDC_TAB_BOTTOM,
                                g_hMainInstance,
                                NULL);
    ISNULL_GOTO(hTabBottom, error_return);

    pstTabPageInfo->hMainTab = hTab;
    pstTabPageInfo->hEditDisass = hEditDisass;
    pstTabPageInfo->hListCallStack = hListCStack;
    pstTabPageInfo->hListRegisters = hListRegisters;
    pstTabPageInfo->hListThreads = hListThreads;
    pstTabPageInfo->hEditCommand = hEditCommand;
    pstTabPageInfo->hStaticCommand = hStaticCommand;
    pstTabPageInfo->hTabBottom = hTabBottom;
    pstTabPageInfo->iTabIndex = iNewIndex;
    pstTabPageInfo->hFixedFont = hFixedFont;

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

void vDeleteTabPage(HWND hTab, PTABPAGEINFO pstTabPageInfo)
{
    ASSERT(ISVALID_HANDLE(hTab));
    ASSERT(pstTabPageInfo);
    ASSERT(pstTabPageInfo->iTabIndex >= 0);

    // First, destroy all child windows
    DestroyWindow(pstTabPageInfo->hEditDisass);
    DestroyWindow(pstTabPageInfo->hListCallStack);
    DestroyWindow(pstTabPageInfo->hListRegisters);
    DestroyWindow(pstTabPageInfo->hListThreads);
    DestroyWindow(pstTabPageInfo->hStaticCommand);
    DestroyWindow(pstTabPageInfo->hEditCommand);
    DestroyWindow(pstTabPageInfo->hTabBottom);

    // Delete the font GDI object
    DeleteObject(pstTabPageInfo->hFixedFont);

    // Remove the tab item
    SendMessage(hTab, TCM_DELETEITEM, (int)pstTabPageInfo->iTabIndex, (LPARAM)NULL);
}

BOOL fSetTabPageText(HWND hTab, int iTabIndex, PWCHAR pszTabText)
{
    ASSERT(ISVALID_HANDLE(hTab));
    ASSERT(iTabIndex >= 0);
    ASSERT(pszTabText);

    TCITEM stTcItem = {0};

    stTcItem.mask = TCIF_TEXT;
    stTcItem.pszText = pszTabText;

    return SendMessage(hTab, TCM_SETITEM, iTabIndex, (LPARAM)&stTcItem);
}

BOOL fGetFixedWidthFont(HWND hWindow, HFONT *phFont)
{
    ASSERT(phFont);

    LOGFONT stLogFont = {0};
    struct _FontSelection stFontSel;

    stLogFont.lfCharSet = ANSI_CHARSET;
    stLogFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    HRESULT hr = StringCchCopy(stLogFont.lfFaceName, LF_FACESIZE, L"Consolas");
    if(FAILED(hr))
    {
        logerror(pstLogger, L"%s(): StringCchCopy() failed %u.", __FUNCTIONW__, GetLastError());
        return FALSE;
    }

    // 1. EnumFontFamilies to enumerate all styles
    int ret = EnumFontFamiliesEx(GetDC(hWindow), &stLogFont, EnumFontCallback, (LPARAM)&stFontSel, 0);
    if(ret != 0)
    {
        logerror(pstLogger, L"%s(): Could not select requested font %u.", __FUNCTIONW__, GetLastError());
        return FALSE;
    }

    // 2. Populate LOGFONT and call CreateFontIndirect
    stFontSel.stLogFont.lfWeight = FW_NORMAL;
    stFontSel.stLogFont.lfHeight = -MulDiv(10, GetDeviceCaps(GetDC(hWindow), LOGPIXELSY), 72);
    stFontSel.stLogFont.lfWidth = 0;

    HFONT hf = CreateFontIndirect(&stFontSel.stLogFont);
    if(!hf)
    {
        logerror(pstLogger, L"%s(): CreateFontIndirect() failed %u.", __FUNCTIONW__, GetLastError());
        return FALSE;
    }

    *phFont = hf;
    return TRUE;
}

int CALLBACK EnumFontCallback(
    const LOGFONT *lpelfe,
    const TEXTMETRIC *lpntme,
    DWORD FontType,
    LPARAM lParam)
{
    // Select a normal non-italic style
    if(lpelfe->lfWeight == FW_NORMAL && lpelfe->lfItalic != TRUE)
    {
        memcpy(&((struct _FontSelection*)lParam)->stLogFont, lpelfe, sizeof(*lpelfe));
        memcpy(&((struct _FontSelection*)lParam)->stTextMetric, lpntme, sizeof(*lpntme));
        
        return 0;
    }

    return 1;
}
