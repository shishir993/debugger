
#ifndef _LOGGER_H
#define _LOGGER_H

#include <Windows.h>
#include <stdio.h>
#include <time.h>

#include "Assert.h"
#include "StringLengths.h"
#include "..\Res\resource.h"

typedef struct _Logger
{
    HANDLE hMutex;
    HANDLE hLogFile;
    WCHAR szMutexName[SLEN_COMMON];
}LOGGER;

BOOL fInitializeLogger(WCHAR *pszLogFilepath, __out LOGGER *pLogger);
void vTerminateLogger(LOGGER *pLogger);
void vWriteLog(LOGGER *pLogger, const WCHAR* pszMessage);
void vWriteLog(LOGGER *pLogger, const WCHAR* pszMessage, int nLen);

#endif // _LOGGER_H
