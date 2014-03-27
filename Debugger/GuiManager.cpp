
#include "Inc\GuiManager.h"

extern PLOGGER pstLogger;

CHL_HTABLE *pTabThreadMap = NULL;

// File local functions
static BOOL fDisplayActiveProcesses(HWND hProcIDList, DWORD **paProcIDs, DWORD *pnProcIDs, DWORD *pnItemsShown);

BOOL fGuiInitialize(__out DWORD *pdwErrCode)
{
    DBG_UNREFERENCED_PARAMETER(pdwErrCode);

    if(pTabThreadMap)
    {
        fChlDsDestroyHT(pTabThreadMap);
    }

    if(!fChlDsCreateHT(&pTabThreadMap, iChlDsGetNearestTableSizeIndex(5), HT_KEY_DWORD, HT_VAL_DWORD, FALSE))
    {
        vWriteLog(pstLogger, L"fGuiInitialize(): Cannot create tab thread map");
        goto error_return;
    }

    ASSERT(pTabThreadMap);
    return TRUE;

    error_return:
    return FALSE;
}

void vGuiExit()
{
    if(pTabThreadMap)
    {
        fChlDsDestroyHT(pTabThreadMap);
    }
}

BOOL fGuiAddTab(int tabIndex, DWORD threadId, __out DWORD *pdwErrCode)
{
    ASSERT(pTabThreadMap);
    ASSERT(tabIndex >= 0);

    DBG_UNREFERENCED_PARAMETER(pdwErrCode);
    
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

    DBG_UNREFERENCED_PARAMETER(pdwErrCode);

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

    DBG_UNREFERENCED_PARAMETER(pdwErrCode);

    DWORD dwThreadId;
    int iValSize;

#ifdef _DEBUG
    WCHAR szDbgMessage[SLEN_LOGLINE];
    swprintf_s(szDbgMessage, _countof(szDbgMessage), L"fGuiFindTab(): %d", tabIndex);
    vWriteLog(pstLogger, szDbgMessage); 
#endif

    if(!fChlDsFindHT(pTabThreadMap, &tabIndex, sizeof(tabIndex), &dwThreadId, &iValSize))
        return FALSE;

    *pdwThreadId = dwThreadId;
    return TRUE;
}

// fGuiGetOpenFilename()
// Just a wrapper for the GetOpenFileName() API
//
BOOL fGuiGetOpenFilename(HWND hMainWindow, WCHAR *pszFilters, __out WCHAR *pszFilepath, DWORD dwBufSize, __out DWORD *pdwErrCode)
{
    ASSERT(ISVALID_HANDLE(hMainWindow));
    ASSERT(pszFilters);
    ASSERT(pszFilepath);

    DBG_UNREFERENCED_PARAMETER(pdwErrCode);

    OPENFILENAME ofn;

    DWORD dwErrorCode = ERROR_SUCCESS;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = pszFilters;
    ofn.nFilterIndex = 1;

    pszFilepath[0] = 0;
    ofn.lpstrFile = pszFilepath;
    ofn.nMaxFile = dwBufSize;
    ofn.Flags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NONETWORKBUTTON;

    if(!GetOpenFileName(&ofn))
    {
        dwErrorCode = CommDlgExtendedError();
        goto error_return;
    }
    return TRUE;

error_return:
    pszFilepath[0] = 0;
    IFPTR_SETVAL(pdwErrCode, dwErrorCode);
    return FALSE;
}

// fGetProcIdDP()
//
BOOL CALLBACK fGetProcIdDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hProcIDList;
    static DWORD *paProcIDs;
    static DWORD nProcIDs;
    static DWORD nListItems;

    static DWORD *pdwProcessIdToReturn = NULL;
    
    static int iSelectedListItem;
    LPNMLISTVIEW lpNMListView = NULL;

    switch(message)
	{
		case WM_INITDIALOG:
		{
            LVCOLUMN lvColumn = {0};

            paProcIDs = NULL;
            nProcIDs = 0;
            iSelectedListItem = -1;
            
            // We should get the address where to store the PID chosen by user, from main window
            ASSERT(lParam);
            pdwProcessIdToReturn = (DWORD*)lParam;

			fChlGuiCenterWindow(hDlg);

            hProcIDList = GetDlgItem(hDlg, IDC_LIST_PROCIDS);

            // List view headers
			lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

			// first column
			lvColumn.pszText = L"Index";
			lvColumn.cx = 0x30;
			SendMessage(hProcIDList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

			// second column
			lvColumn.pszText = L"Process ID"; 
			lvColumn.cx = 0x60;
			SendMessage(hProcIDList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
			
			// third column
			lvColumn.pszText = L"Process Name";                            
			lvColumn.cx = 0x90;
			SendMessage(hProcIDList, LVM_INSERTCOLUMN, 2, (LPARAM)&lvColumn);

            // populate list with active processes' ID
            if(!fDisplayActiveProcesses(hProcIDList, &paProcIDs, &nProcIDs, &nListItems))
            {
                MessageBox(hDlg, L"Cannot display active processes", L"Error", MB_ICONERROR);
                SendMessage(hDlg, WM_CLOSE, 0, 0);
            }

            return TRUE;
        }

        case WM_COMMAND:
		{
			// handle command sent from child window controls
			switch(LOWORD(wParam))
			{
                case IDB_OPENPROC:
                {
                    if(iSelectedListItem < 0 || iSelectedListItem > nListItems)
                    {
                        MessageBox(hDlg, L"You did not select a process to debug", L"No Process", MB_ICONMASK);
                    }
                    else
                    {
                        LVITEM lvSelItem = {0};
                        WCHAR wsProcID[11];

                        lvSelItem.iItem = iSelectedListItem;
                        lvSelItem.iSubItem = 1;
                        lvSelItem.mask = LVIF_TEXT;
                        lvSelItem.pszText = wsProcID;
                        lvSelItem.cchTextMax = _countof(wsProcID);

                        if(!SendDlgItemMessage(hDlg, IDC_LIST_PROCIDS, LVM_GETITEM, 0, (LPARAM)&lvSelItem))
                        {
                            MessageBox(hDlg, L"Error retrieving selected process's PID", L"Error", MB_OK);
                            *pdwProcessIdToReturn = 0;
                        }
                        else
                        {
                            *pdwProcessIdToReturn = _wtoi(lvSelItem.pszText);
                        }

                        SendMessage(hDlg, WM_CLOSE, 0, 0);
                    }
                    return TRUE;
                }

                case IDB_REFRESH:
                {
                    iSelectedListItem = -1;
                    // populate list with active processes' ID
                    if(!fDisplayActiveProcesses(hProcIDList, &paProcIDs, &nProcIDs, &nListItems))
                    {
                        MessageBox(hDlg, L"Cannot display active processes", L"Error", MB_ICONERROR);
                        SendMessage(hDlg, WM_CLOSE, 0, 0);
                    }
                    return TRUE;
                }
                
                case IDB_CANCEL:
                {
                    SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
                }
            }

            break;
        
        }// WM_COMMAND

        case WM_NOTIFY:
        {
            switch ( ((LPNMHDR)lParam)->code )
			{
                case LVN_ITEMCHANGED:
				{
					if( (lpNMListView = (LPNMLISTVIEW)lParam) == NULL )
					{
						// error
                        dbgwprintf(L"%s(): LVN_ITEMCHANGED: lParam is NULL\n", __FUNCTIONW__);
						return 0;
					}

					if( lpNMListView->uNewState & LVIS_SELECTED )
						if( lpNMListView->iItem != -1 )
							iSelectedListItem = lpNMListView->iItem;
					
					return 0;	// actually, no return value is required
				}
            }

            break;

        }// WM_NOTIFY

        case WM_CLOSE:
		{
            if(paProcIDs)
            {
                vChlMmFree((void**)&paProcIDs);
            }

			EndDialog(hDlg, 0);
			return TRUE;
		}

    }
    return FALSE;
}// fGetProcIdDP()

// fDisplayActiveProcesses()
//
static BOOL fDisplayActiveProcesses(HWND hProcIDList, DWORD **paProcIDs, DWORD *pnProcIDs, DWORD *pnItemsShown)
{
    ASSERT(hProcIDList && hProcIDList != INVALID_HANDLE_VALUE);

    DWORD dwBytesReturned = 0;
    LVITEM lvItem;

    WCHAR wsIndex[5];
    WCHAR wsProcID[11];
    WCHAR wsProcName[MAX_PATH+1];

    int iRetVal = 0;
    int nItemsShown = -1;

    BOOL fErrorInDisplaying = FALSE;

    if(!fChlMmAlloc((void**)paProcIDs, 1024*sizeof(DWORD), NULL))
    {
        logerror(pstLogger, L"fDisplayActiveProcesses(): mem alloc failed");
        return FALSE;
    }
    ZeroMemory(*paProcIDs, 1024*sizeof(DWORD));

    // Clear list view
    if(!SendMessage(hProcIDList, LVM_DELETEALLITEMS, 0, 0))
        return FALSE;

    // list processes in the system
    if(!EnumProcesses(*paProcIDs, 1024*sizeof(DWORD), &dwBytesReturned))
    {
        logerror(pstLogger, L"Failed to enumerate processes in the system. Sorry. %d", GetLastError());
        return FALSE;
    }

    logerror(pstLogger, L"Enumerated %d processes", dwBytesReturned/sizeof(DWORD));

    *pnProcIDs = dwBytesReturned/sizeof(DWORD);
    for(DWORD index = 0; index < dwBytesReturned/sizeof(DWORD); ++index)
    {
        if(!fChlPsGetProcNameFromID((*paProcIDs)[index], wsProcName, _countof(wsProcName)))
        {
            dbgwprintf(L"Error reading proc name for %d %d", (*paProcIDs)[index], GetLastError());
            logwarn(pstLogger, L"Error reading proc name for %d %d", (*paProcIDs)[index], GetLastError());
            continue;
        }
        else
        {
            logerror(pstLogger, L"%5d %s", (*paProcIDs)[index], wsProcName);
        }

        // insert into the list
        ZeroMemory(&lvItem, sizeof(lvItem));

        ++nItemsShown;

        // index
        swprintf_s(wsIndex, _countof(wsIndex), L"%d", index);
        lvItem.iItem = nItemsShown;
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = wsIndex;
		lvItem.cchTextMax = wcsnlen_s(wsIndex, _countof(wsIndex)) + 1;
		if( (iRetVal = SendMessage(hProcIDList, LVM_INSERTITEM, 0, (LPARAM)&lvItem)) == -1 )
		{
			// error
			iRetVal = GetLastError();
            logerror(pstLogger, L"fDisplayActiveProcesses(): List insert failed %d", iRetVal);
            fErrorInDisplaying = TRUE;
			break;
		}

        // proc ID
        swprintf_s(wsProcID, _countof(wsProcID), L"%d", (*paProcIDs)[index]);
        lvItem.iSubItem = 1;
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = wsProcID;
		lvItem.cchTextMax = wcsnlen_s(wsProcID, _countof(wsProcID)) + 1;
		if( (iRetVal = SendMessage(hProcIDList, LVM_SETITEM, 0, (LPARAM)&lvItem)) == -1 )
		{
			// error
			iRetVal = GetLastError();
            logerror(pstLogger, L"fDisplayActiveProcesses(): List insert failed %d", iRetVal);
            fErrorInDisplaying = TRUE;
			break;
		}

        // proc name
        lvItem.iSubItem = 2;
		lvItem.mask = LVIF_TEXT;
		lvItem.pszText = wsProcName;
		lvItem.cchTextMax = wcsnlen_s(wsProcName, _countof(wsProcName)) + 1;
		if( (iRetVal = SendMessage(hProcIDList, LVM_SETITEM, 0, (LPARAM)&lvItem)) == -1 )
		{
			// error
			iRetVal = GetLastError();
            logerror(pstLogger, L"fDisplayActiveProcesses(): List insert failed %d", iRetVal);
            fErrorInDisplaying = TRUE;
			break;
		}
    }
    
    *pnItemsShown = nItemsShown;
    return !fErrorInDisplaying;

}// fDisplayActiveProcesses()
