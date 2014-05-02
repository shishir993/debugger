
#include "Inc\GuiManager.h"
#include "Inc\WndProc.h"

extern PLOGGER pstLogger;

static CHL_HTABLE *pTabThreadMap = NULL;

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

// Function to be called when main window is exiting and we want to detach from
// any targets we have attached to.
//
BOOL fOnExitDetachTargets()
{
    CHL_HT_ITERATOR htItr;

    int iTabIndex, keySize, valSize;
    DWORD dwThreadId;
    
    HANDLE hThread = NULL;
    DWORD dwWaitStatus;

    BOOL fRetVal = TRUE;

    GUIDBGCOMM stGuiDbgComm;
    
    if(!fChlDsInitIteratorHT(&htItr))
    {
        logerror(pstLogger, L"%s(): fChlDsInitIteratorHT() failed %u", GetLastError());
        return FALSE;
    }

    logtrace(pstLogger, L"%s(): Iterating through tab list...", __FUNCTIONW__);
    while(fChlDsGetNextHT(pTabThreadMap, &htItr, &iTabIndex, &keySize, &dwThreadId, &valSize))
    {
        ASSERT(keySize == sizeof(int) && valSize == sizeof(DWORD));

        logtrace(pstLogger, L"Retrieved %d %u", iTabIndex, dwThreadId);

        hThread = OpenThread(SYNCHRONIZE, FALSE, dwThreadId);
        if(hThread == NULL)
        {
            fRetVal = FALSE;

            logerror(pstLogger, L"OpenThread() failed %u", GetLastError());
            continue;
        }

        stGuiDbgComm.fFreeThis = FALSE;
        stGuiDbgComm.GD_fDetachOnDebuggerExit = TRUE;

        // Post message and wait for it to terminate
        if(!PostThreadMessage(dwThreadId, GD_SESS_DETACH, 0, (LPARAM)&stGuiDbgComm))
        {
            fRetVal = FALSE;

            logerror(pstLogger, L"PostThreadMessage() failed %u", GetLastError());

            CloseHandle(hThread);
            continue;
        }

        logtrace(pstLogger, L"Waiting for thread %u to exit", dwThreadId);
        dwWaitStatus = WaitForSingleObject(hThread, 5000);
        if(dwWaitStatus != WAIT_OBJECT_0)
        {
            fRetVal = FALSE;

            logwarn(pstLogger, L"Thread %u did not terminate within timeout. Moving on...", dwThreadId);

            // fall through to end of while
        }

        CloseHandle(hThread);
    }

    return fRetVal;
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

    static PDBG_SESSIONSTART pstSessionInfo = NULL;
    
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
            pstSessionInfo = (PDBG_SESSIONSTART)lParam;

            // Set flag to indicate that we will debug an active process
            pstSessionInfo->fDebuggingActiveProcess = TRUE;

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
                            pstSessionInfo->dwTargetPID = 0;
                        }
                        else
                        {
                            pstSessionInfo->dwTargetPID = _wtoi(lvSelItem.pszText);
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
            dbgwprintf(L"Error reading proc name for %d %d\n", (*paProcIDs)[index], GetLastError());
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

// fGetNewProgramDP()
//
BOOL CALLBACK fGetNewProgramDP(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PDBG_SESSIONSTART pstSessionInfo = NULL;
    static WCHAR szFilters[] = L"Executables\0*.exe\0\0";

    HANDLE hFile = NULL;
    WCHAR szTargetPath[SLEN_MAXPATH];

    DWORD dwError = ERROR_SUCCESS;

    switch(message)
	{
		case WM_INITDIALOG:
		{   
            // We should, from main window, get the address where to store the path of target chosen by user
            ASSERT(lParam);
            pstSessionInfo = (PDBG_SESSIONSTART)lParam;
            ZeroMemory(pstSessionInfo, sizeof(DBG_SESSIONSTART));

			fChlGuiCenterWindow(hDlg);

            return TRUE;
        }

        case WM_COMMAND:
		{
			// handle command sent from child window controls
			switch(LOWORD(wParam))
			{
                case IDC_NP_BROWSE:
                {
                    // Get the path to the target program to be debugged
                    if(!fGuiGetOpenFilename(hDlg, szFilters, szTargetPath, sizeof(szTargetPath), &dwError))
                    {
                        vWriteLog(pstLogger, L"%s(): fGuiGetOpenFilename failed: %u", __FUNCTIONW__, dwError);
                        return 0;
                    }

                    // Check if the image file exists and can be read
                    hFile = CreateFile(szTargetPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if(hFile == INVALID_HANDLE_VALUE)
                    {
                        MessageBox(hDlg, L"Chosen target path not found or access denied. See log file.", L"Error", MB_ICONEXCLAMATION);

                        SET_ERRORCODE(dwError);
                        vWriteLog(pstLogger, L"%s(): CreateFile failed: %u", __FUNCTIONW__, dwError);
                        return 0;
                    }
                    CloseHandle(hFile);

                    // Insert path into the edit control
                    SendDlgItemMessage(hDlg, IDC_NP_PROGRAMPATH, WM_SETTEXT, 0, (LPARAM)szTargetPath);

                    return TRUE;
                }
                
                case IDC_NP_OK:
                {
                    // Get the text from edit control
                    int nLen = SendDlgItemMessage(hDlg, IDC_NP_PROGRAMPATH, WM_GETTEXTLENGTH, 0, NULL);

                    if(nLen < 0)
                    {
                        // TODO: show error. log it.

                        // Close the dialog
                        ZeroMemory(pstSessionInfo, sizeof(DBG_SESSIONSTART));
                        SendMessage(hDlg, WM_CLOSE, 0, 0);
                    }
                    else if(nLen == 0)
                    {
                        MessageBox(hDlg, L"Specify path or click Cancel", L"Info", MB_ICONEXCLAMATION);
                    }
                    else if(nLen > SLEN_MAXPATH - 1)
                    {
                        MessageBox(hDlg, L"Path too long!", L"Error", MB_ICONEXCLAMATION);
                        logwarn(pstLogger, L"%s(): Target path length too long %d/%d", __FUNCTIONW__, nLen, SLEN_MAXPATH-1);
                    }
                    else
                    {
                        // Returns number of chars copied
                        nLen = SendDlgItemMessage(hDlg, IDC_NP_PROGRAMPATH, WM_GETTEXT, _countof(pstSessionInfo->szTargetPath), (LPARAM)pstSessionInfo->szTargetPath);
                        if(nLen <= 0)
                        {
                            MessageBox(hDlg, L"Error reading specified target path. See log file.", L"Error", MB_ICONERROR);
                            logwarn(pstLogger, L"%s(): WM_GETTEXT failed %u", __FUNCTIONW__, GetLastError());
                        }
                        else
                        {
                            // We have copied the path, now get state of the checkbox
                            if(SendDlgItemMessage(hDlg, IDC_NP_BREAKMAIN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            {
                                pstSessionInfo->fBreakAtMain = TRUE;
                            }

                            SendMessage(hDlg, WM_CLOSE, 0, 0);
                        }
                    }
                    return TRUE;
                }

                case IDC_NP_CANCEL:
                {
                    ZeroMemory(pstSessionInfo, sizeof(DBG_SESSIONSTART));
                    SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
                }
            }

            break;
        
        }// WM_COMMAND

        case WM_CLOSE:
		{
			EndDialog(hDlg, 0);
			return TRUE;
		}

    }

    return FALSE;
}

BOOL fGuiUpdateThreadsList(HWND hThreadListView, PLV_THREADINFO pstLvThreadInfo, int nItems)
{
    ASSERT(ISVALID_HANDLE(hThreadListView));
    ASSERT(pstLvThreadInfo);
    ASSERT(nItems > 0 && nItems < 16000);   // a reasonable limit, dont'cha think?

    WCHAR szThreadId[SLEN_INT16];
    WCHAR szEipLocation[SLEN_DWORDSTR_HEX];
    WCHAR szFunctionName[SLEN_COMMON128];
    WCHAR szType[SLEN_COMMON32];
    WCHAR szPriority[SLEN_INT16];

    int index;
    WCHAR **ppszStrings;

    // Allocate memory to hold WCHAR pointers to the 5 items to display
    if(!fChlMmAlloc((void**)&ppszStrings, sizeof(WCHAR*) * LV_THREAD_NUMCOLUMNS, NULL))
    {
        return FALSE;
    }

    ASSERT(LV_THREAD_NUMCOLUMNS == 5);

    ppszStrings[0] = szThreadId;
    ppszStrings[1] = szEipLocation;
    ppszStrings[2] = szFunctionName;
    ppszStrings[3] = szType;
    ppszStrings[4] = szPriority;

    // TODO: log failure as error
    ListView_DeleteAllItems(hThreadListView);

    for(index = 0; index < nItems; ++index)
    {
        // Construct the strings
        swprintf_s(szThreadId, _countof(szThreadId), L"%u", pstLvThreadInfo[index].dwThreadId);
        swprintf_s(szEipLocation, _countof(szEipLocation), L"0x%08x", pstLvThreadInfo[index].dwEIPLocation);
        swprintf_s(szFunctionName, _countof(szFunctionName), L"0x%08x", pstLvThreadInfo[index].szFunction);

        ASSERT(pstLvThreadInfo[index].thType == THTYPE_MAIN || pstLvThreadInfo[index].thType == THTYPE_WORKER);
        swprintf_s(szType, _countof(szType), L"%s", aszThreadTypes[pstLvThreadInfo[index].thType]);

        swprintf_s(szPriority, _countof(szPriority), L"%d", pstLvThreadInfo[index].iThreadPri);

        // Insert into list view
        if(!fChlGuiAddListViewRow(hThreadListView, ppszStrings, LV_THREAD_NUMCOLUMNS))
        {
            logerror(pstLogger, L"%s(): fChlGuiAddListViewRow failed %u", __FUNCTIONW__, GetLastError());
            return FALSE;
        }
    }

    vChlMmFree((void**)&ppszStrings);

    return TRUE;
}

BOOL fGuiUpdateRegistersList(HWND hRegsListView, WCHAR *apszNames[], DWORD *padwValues, int nItems)
{
    ASSERT(ISVALID_HANDLE(hRegsListView));
    ASSERT(apszNames);
    ASSERT(padwValues);
    ASSERT(nItems > 0);

    int index;
    WCHAR **ppszStrings;

    WCHAR szName[SLEN_REGISTER_NAME];
    WCHAR szValue[SLEN_DWORDSTR_HEX];

    // Allocate memory to hold WCHAR pointers to the items to display
    if(!fChlMmAlloc((void**)&ppszStrings, sizeof(WCHAR*) * LV_REGS_NUMCOLUMNS, NULL))
    {
        return FALSE;
    }

    ASSERT(LV_REGS_NUMCOLUMNS == 2);

    ppszStrings[0] = szName;
    ppszStrings[1] = szValue;

    // TODO: log failure as error
    ListView_DeleteAllItems(hRegsListView);

    for(index = 0; index < nItems; ++index)
    {
        swprintf_s(szName, _countof(szName), L"%s", apszNames[index]);
        swprintf_s(szValue, _countof(szValue), L"0x%08x", padwValues[index]);

        // Insert into list view
        if(!fChlGuiAddListViewRow(hRegsListView, ppszStrings, LV_REGS_NUMCOLUMNS))
        {
            logerror(pstLogger, L"%s(): fChlGuiAddListViewRow failed %u", __FUNCTIONW__, GetLastError());
            return FALSE;
        }
    }

    vChlMmFree((void**)&ppszStrings);

    return TRUE;
}
