
#ifndef _LOGGER_H
#define _LOGGER_H

#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <time.h>

#include "Assert.h"
#include "StringLengths.h"
#include "..\Res\resource.h"

typedef struct _Logger
{
    HANDLE hMutex;
    HANDLE hLogFile;
    WCHAR szMutexName[SLEN_COMMON64];
}LOGGER, *PLOGGER;

BOOL fInitializeLogger(WCHAR *pszLogFilepath, __out LOGGER *pLogger);
void vTerminateLogger(LOGGER *pLogger);
void vWriteLog(LOGGER *pLogger, const WCHAR* pszMessageFmt, ...);

#endif // _LOGGER_H
