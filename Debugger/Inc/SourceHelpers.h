
#ifndef _CODECHECKERS_H
#define _CODECHECKERS_H

#include <Windows.h>

#define ISNULL(PtrHandle)                       (PtrHandle == NULL)
#define ISNULL_GOTO(PtrHandle, gotoLocation)    if(ISNULL(PtrHandle)) { goto gotoLocation; }
#define ISVALID_HANDLE(handle)                  (handle != NULL && handle != INVALID_HANDLE_VALUE)

#define SET_ERRORCODE(dwErrCode)                { dwErrCode = GetLastError(); }
#define SET_ERRORCODE_PTR(pdwErrCode)           { if(!ISNULL(pdwErrCode)) { *pdwErrCode = GetLastError(); } }

#endif // _CODECHECKERS_H
