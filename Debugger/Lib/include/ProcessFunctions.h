
// ProcessFunctions.h
// Contains functions that provide IO operation services
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      03/25/14 Initial version
//      09/09/14 Refactor to store defs in individual headers.
//      09/12/14 Naming convention modifications
//

#ifndef _PROCESSFUNCTIONS_H
#define _PROCESSFUNCTIONS_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"

// Custom error codes
#define CHLE_PROC_DOSHEADER     17150
#define CHLE_PROC_TEXTSECHDR    17151
#define CHLE_PROC_NOEXEC        17152

// -------------------------------------------
// Functions exported

DllExpImp HRESULT CHL_PsGetProcNameFromID(_In_ DWORD pid, _Inout_z_ WCHAR *pwsProcName, _In_ DWORD dwBufSize);
DllExpImp HRESULT CHL_PsGetNtHeaders(_In_ HANDLE hMapView, _Out_ PIMAGE_NT_HEADERS *ppstNtHeaders);
DllExpImp HRESULT CHL_PsGetPtrToCode(
    _In_ DWORD dwFileBase, 
    _In_ PIMAGE_NT_HEADERS pNTHeaders, 
    _Out_ PDWORD pdwCodePtr, 
    _Out_ PDWORD pdwSizeOfData,
    _Out_ PDWORD pdwCodeSecVirtAddr);

DllExpImp HRESULT CHL_PsGetEnclosingSectionHeader(_In_ DWORD rva, _In_ PIMAGE_NT_HEADERS pNTHeader, _Out_ PIMAGE_SECTION_HEADER *ppstSecHeader);

#ifdef __cplusplus
}
#endif

#endif // _PROCESSFUNCTIONS_H
