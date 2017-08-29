
// General.h
// Contains general helper functions
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      01/20/14 Initial version
//      09/12/14 Naming convention modifications
//

#ifndef _GENERAL_H
#define _GENERAL_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"

// -------------------------------------------
// Functions exported

#if !defined(_WIN64)

DllExpImp BOOL CHL_GnIsOverflowINT(_In_ int a, _In_ int b);
DllExpImp BOOL CHL_GnIsOverflowUINT(_In_ UINT a, _In_ UINT b);

#endif // _WIN64

DllExpImp HRESULT CHL_GnCreateMemMapOfFile(
    _In_ HANDLE hFile, 
    _In_ DWORD dwReqProtection, 
    _Out_ PHANDLE phMapObj, 
    _Out_ PHANDLE phMapView);

// -------------------------------------------
// Functions used only within this library

HRESULT CHL_GnOwnMutex(_In_ HANDLE hMutex);

#ifdef __cplusplus
}
#endif

#endif // _GENERAL_H
