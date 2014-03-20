
#ifndef _LOGGER_H
#define _LOGGER_H

#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <time.h>

#include "Assert.h"
#include "StringLengths.h"
#include "..\Res\resource.h"

#define LOGTYPE_TRACE   0
#define LOGTYPE_WARN    1
#define LOGTYPE_ERROR   2

#define logtrace(plogger, fmt, ...)     vWriteLogType(LOGTYPE_TRACE, plogger, fmt, __VA_ARGS__)
#define logwarn(plogger, fmt, ...)      vWriteLogType(LOGTYPE_WARN, plogger, fmt, __VA_ARGS__)
#define logerror(plogger, fmt, ...)     vWriteLogType(LOGTYPE_ERROR, plogger, fmt, __VA_ARGS__)

#ifdef _DEBUG
    #define dbgwprintf(fmt, ...)        wprintf(fmt, __VA_ARGS__)
#else
    #define dbgwprintf(fmt, ...)
#endif

typedef struct _Logger
{
    HANDLE hMutex;
    HANDLE hLogFile;
    WCHAR szMutexName[SLEN_COMMON64];
}LOGGER, *PLOGGER;

BOOL fInitializeLogger(WCHAR *pszLogFilepath, __out LOGGER *pLogger);
void vTerminateLogger(LOGGER *pLogger);
void vWriteLog(LOGGER *pLogger, const WCHAR* pszMessageFmt, ...);
void vWriteLogType(int iLogLevel, PLOGGER pLogger, const WCHAR *pszMessageFmt, ...);

#endif // _LOGGER_H
