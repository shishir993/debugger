
#include "Inc\GuiManager.h"

extern PLOGGER pstLogger;

CHL_HTABLE *pTabThreadMap = NULL;

BOOL fGuiInitialize(__out DWORD *pdwErrCode)
{
    if(pTabThreadMap)
    {
        fChlDsDestroyHT(pTabThreadMap);
    }

    if(!fChlDsCreateHT(&pTabThreadMap, iChlDsGetNearestTableSizeIndex(5), HT_KEY_DWORD, HT_VAL_DWORD))
    {
        vWriteLog(pstLogger, L"fGuiInitialize(): Cannot create tab thread map");
        goto error_return;
    }

    ASSERT(pTabThreadMap);
    return TRUE;

    error_return:
    return FALSE;
}

BOOL fGuiAddTab(int tabIndex, DWORD threadId, __out DWORD *pdwErrCode)
{
    ASSERT(pTabThreadMap);
    ASSERT(tabIndex >= 0);
    
#ifdef _DEBUG
    WCHAR szDbgMessage[SLEN_LOGLINE];
    swprintf_s(szDbgMessage, _countof(szDbgMessage), L"fGuiAddTab(): %d %u", tabIndex, threadId);
    vWriteLog(pstLogger, szDbgMessage); 
#endif

    return fChlDsInsertHT(pTabThreadMap, &tabIndex, sizeof(tabIndex), &threadId, sizeof(threadId));
}

BOOL fGuiRemTab(int tabIndex, __out DWORD *pdwErrCode)
{
    ASSERT(pTabThreadMap);
    ASSERT(tabIndex >= 0);

#ifdef _DEBUG
    WCHAR szDbgMessage[SLEN_LOGLINE];
    swprintf_s(szDbgMessage, _countof(szDbgMessage), L"fGuiRemTab(): %d", tabIndex);
    vWriteLog(pstLogger, szDbgMessage); 
#endif

    return fChlDsRemoveHT(pTabThreadMap, &tabIndex, sizeof(tabIndex));
}

BOOL fGuiFindTab(int tabIndex, __out DWORD *pdwThreadId, __out DWORD *pdwErrCode)
{
    ASSERT(pTabThreadMap);
    ASSERT(tabIndex >= 0);
    ASSERT(pdwThreadId);

    DWORD dwThreadId;
    int iValSize;

#ifdef _DEBUG
    WCHAR szDbgMessage[SLEN_LOGLINE];
    swprintf_s(szDbgMessage, _countof(szDbgMessage), L"fGuiFindTab(): %d", tabIndex);
    vWriteLog(pstLogger, szDbgMessage); 
#endif

    if(!fChlDsFindHT(pTabThreadMap, &tabIndex, sizeof(tabIndex), &dwThreadId, &iValSize))
        return FALSE;

    pdwThreadId = &dwThreadId;
    return TRUE;
}

BOOL fGuiGetOpenFilename(HWND hMainWindow, WCHAR *pszFilters, __out WCHAR **ppszFilepath, __out DWORD *pdwErrCode)
{
    ASSERT(ISVALID_HANDLE(hMainWindow));
    ASSERT(pszFilters);

    OPENFILENAME ofn;
    DWORD dwFlags = 0;
    HANDLE hFile = NULL;

    DWORD dwErrorCode = ERROR_SUCCESS;
    WCHAR szLogMessage[SLEN_LOGLINE];

    WCHAR *pszPath = NULL;

    if(!fChlMmAlloc((void**)&pszPath, CONV_WCHARSIZE(SLEN_MAXPATH), &dwErrorCode))
    {
        swprintf_s(szLogMessage, _countof(szLogMessage), L"%s(): fChlMmAlloc failed: %u", __FUNCTIONW__, dwErrorCode);
        vWriteLog(pstLogger, szLogMessage);
        goto error_return;
    }

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = pszFilters;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = pszPath;
    ofn.nMaxFile = CONV_WCHARSIZE(SLEN_MAXPATH);
    ofn.Flags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NONETWORKBUTTON;

    if(!GetOpenFileName(&ofn))
    {
        dwErrorCode = CommDlgExtendedError();
        goto error_return;
    }

    *ppszFilepath = pszPath;
    return TRUE;

    error_return:
    IFPTR_FREE(pszPath);
    return FALSE;

}
