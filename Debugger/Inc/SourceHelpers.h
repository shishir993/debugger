
#ifndef _CODECHECKERS_H
#define _CODECHECKERS_H

#include <Windows.h>

#define ISNULL(PtrHandle)                       (PtrHandle == NULL)
#define ISNULL_GOTO(PtrHandle, gotoLocation)    if(ISNULL(PtrHandle)) { goto gotoLocation; }
#define ISVALID_HANDLE(handle)                  (handle != NULL && handle != INVALID_HANDLE_VALUE)

#define IFPTR_FREE(ptr)                         if(!ISNULL(ptr)) { vChlMmFree((void**)&ptr); ptr = NULL; }
#define FREE_HANDLE(handle)                     if(ISVALID_HANDLE(handle)) { CloseHandle(handle); } handle = NULL

#define IFPTR_SETVAL(ptr, val)                     if(ptr) { *ptr = val; }
#define SET_ERRORCODE(dwErrCode)                { dwErrCode = GetLastError(); }
#define SET_ERRORCODE_PTR(pdwErrCode)           { if(!ISNULL(pdwErrCode)) { *pdwErrCode = GetLastError(); } }

#define CONV_WCHARSIZE(bytes)                   (bytes * sizeof(WCHAR))
#define CONV_BYTES_wcsnlen_s(wstring, maxcount) ( (wcsnlen_s(wstring, maxcount) + 1) * sizeof(WCHAR) )

#endif // _CODECHECKERS_H
