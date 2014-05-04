
#include "Inc\Logger.h"

#define MAX_LOGLINE     (SLEN_LOGLINE + 24)     // accounting for the extra time info length and logtype

static void vWriteLogInternal(int iLogType, LOGGER *pLogger, const WCHAR* pszMessage, int nLen);

BOOL fInitializeLogger(WCHAR *pszLogFilepath, __out LOGGER *pLogger)
{
    HANDLE hMutex = NULL;
    WCHAR szMutexNameTemp[SLEN_COMMON64];

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

// Writes a line of log to the logger. Uses LOGTYPE_TRACE to log the line.
// 
void vWriteLog(LOGGER *pLogger, const WCHAR* pszMessageFmt, ...)
{
    ASSERT(pszMessageFmt);

    va_list pArgs;
    WCHAR szLogMessage[SLEN_LOGLINE];

    HRESULT hrReturn = S_OK;
    
    va_start(pArgs, pszMessageFmt);
    hrReturn = StringCchVPrintf(szLogMessage, _countof(szLogMessage), pszMessageFmt, pArgs);
    va_end(pArgs);

    if(FAILED(hrReturn))
    {
        // todo:
        return;
    }

    vWriteLogInternal(LOGTYPE_TRACE, pLogger, szLogMessage, wcslen(szLogMessage));
    return;
}

// Function that allows one to specify the log type to be written.
// TRACE, WARN or ERROR types. The corresponding string will be appended to the 
// beginning of the log line.
// ** Added a new function instead of changing the vWriteLog() to prevent breaking
// ** existing callers.
void vWriteLogType(int iLogLevel, PLOGGER pLogger, const WCHAR *pszMessageFmt, ...)
{
    ASSERT(pszMessageFmt);
    ASSERT(iLogLevel >= LOGTYPE_TRACE && iLogLevel <= LOGTYPE_ERROR);

    va_list pArgs;
    WCHAR szLogMessage[SLEN_LOGLINE];

    HRESULT hrReturn = S_OK;
    
    va_start(pArgs, pszMessageFmt);
    hrReturn = StringCchVPrintf(szLogMessage, _countof(szLogMessage), pszMessageFmt, pArgs);
    va_end(pArgs);

    if(FAILED(hrReturn))
    {
        // todo:
        return;
    }

    vWriteLogInternal(iLogLevel, pLogger, szLogMessage, wcslen(szLogMessage));
}

// Internal function that actually does the writing to log file part
//
static void vWriteLogInternal(int iLogLevel, LOGGER *pLogger, const WCHAR* pszMessage, int nLen)
{
    ASSERT(pLogger);
    ASSERT(pszMessage);
    ASSERT(nLen > 1);

    SYSTEMTIME stCurrentTime;
    WCHAR szLogMessage[MAX_LOGLINE];

    DWORD dwLogChars;
    DWORD dwWritten;

    GetLocalTime(&stCurrentTime);

    // todo: read only nLen chars from pszMessage
	
	// Construct the string to be displayed
    switch(iLogLevel)
    {
        case LOGTYPE_TRACE:
        {
            swprintf_s(szLogMessage, MAX_LOGLINE, L"[%02d:%02d:%02d.%03d] TRACE: %s\r\n", stCurrentTime.wHour, 
		        stCurrentTime.wMinute, stCurrentTime.wSecond, stCurrentTime.wMilliseconds, pszMessage);
            break;
        }

        case LOGTYPE_WARN:
        {
            swprintf_s(szLogMessage, MAX_LOGLINE, L"[%02d:%02d:%02d.%03d] WARN : %s\r\n", stCurrentTime.wHour, 
		        stCurrentTime.wMinute, stCurrentTime.wSecond, stCurrentTime.wMilliseconds, pszMessage);
            break;
        }

        case LOGTYPE_ERROR:
        {
            swprintf_s(szLogMessage, MAX_LOGLINE, L"[%02d:%02d:%02d.%03d] ERROR: %s\r\n", stCurrentTime.wHour, 
		        stCurrentTime.wMinute, stCurrentTime.wSecond, stCurrentTime.wMilliseconds, pszMessage);
            break;
        }

        default:
            ASSERT(FALSE);
            break;
    }
	
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
