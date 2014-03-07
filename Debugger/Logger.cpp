
#include "Inc\Logger.h"

BOOL fInitializeLogger(WCHAR *pszLogFilepath, __out LOGGER *pLogger)
{
    HANDLE hMutex = NULL;
    WCHAR szMutexNameTemp[SLEN_COMMON];

    HANDLE hFile = NULL;

    ASSERT(pszLogFilepath);
    ASSERT(pLogger);

    // todo: check return value
    LoadString(GetModuleHandle(NULL), IDS_LOGGERMUTEX, szMutexNameTemp, _countof(szMutexNameTemp) - SLEN_INT16);

    memset(pLogger, 0, sizeof(LOGGER));

    srand(time(NULL));
    swprintf_s(pLogger->szMutexName, _countof(pLogger->szMutexName), L"%s_%d", szMutexNameTemp,  rand() % 65536);
    if(!(hMutex = CreateMutex(NULL, true, pLogger->szMutexName)))
    {
        // todo: do something similar to throw exception
        goto error_return;
    }
    
    // Create log file
    hFile = CreateFile(pszLogFilepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == NULL)
    {
        // todo: do something similar to throw exception
        goto error_return;
    }

    pLogger->hMutex = hMutex;
    pLogger->hLogFile = hFile;

    ReleaseMutex(hMutex);
    vWriteLog(pLogger, L"Logger Initialized");
    return TRUE;

    error_return:
    if(hFile)
    {
        CloseHandle(hFile);
    }

    if(hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    memset(pLogger, 0, sizeof(LOGGER));
    return FALSE;
}

void vTerminateLogger(LOGGER *pLogger)
{
    ASSERT(pLogger);
    ASSERT(pLogger->hMutex);
    ASSERT(pLogger->hLogFile);

    if(WaitForSingleObject(pLogger->hMutex, INFINITE) != WAIT_OBJECT_0)
    {
        // todo: something like an exception. SEH?
        return;
    }
    vWriteLog(pLogger, L"Logger Terminating...");
    CloseHandle(pLogger->hLogFile);
    CloseHandle(pLogger->hMutex);

    // Mutex is never released

    memset(pLogger, 0, sizeof(LOGGER));
    return;
}

void vWriteLog(LOGGER *pLogger, const WCHAR* pszMessage)
{
    ASSERT(pszMessage);

    vWriteLog(pLogger, pszMessage, wcslen(pszMessage));
    return;
}

void vWriteLog(LOGGER *pLogger, const WCHAR* pszMessage, int nLen)
{
    ASSERT(pLogger);
    ASSERT(pszMessage);
    ASSERT(nLen > 1);

    SYSTEMTIME stCurrentTime;
    WCHAR szLogMessage[SLEN_LOGLINE];

    DWORD dwLogChars;
    DWORD dwWritten;

    GetLocalTime(&stCurrentTime);
	
	// Construct the string to be displayed
	swprintf_s(szLogMessage, SLEN_LOGLINE, L"[%02d:%02d:%02d.%03d] %s\r\n", stCurrentTime.wHour, 
		        stCurrentTime.wMinute, stCurrentTime.wSecond, stCurrentTime.wMilliseconds, pszMessage);

    ASSERT(pLogger->hLogFile);
    ASSERT(pLogger->hMutex);

    // Get mutex
    if(WaitForSingleObject(pLogger->hMutex, INFINITE) != WAIT_OBJECT_0)
    {
        // todo: something like an exception. SEH?
        return;
    }

    dwLogChars = wcslen(szLogMessage) * sizeof(WCHAR);
    if(!WriteFile(pLogger->hLogFile, szLogMessage, dwLogChars, &dwWritten, NULL))
    {
        // todo: SEH?
    }
    else if(dwWritten != dwLogChars)
    {
        // todo: SEH?
    }

    ReleaseMutex(pLogger->hMutex);
    return;
}
