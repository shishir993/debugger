
// MemFunctions.h
// Contains functions that provide memory alloc/dealloc services
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      06/23/13 Initial version
//      09/09/14 Refactor to store defs in individual headers.
//      09/12/14 Naming convention modifications
//

#ifndef _MEMFUNCTIONS_H
#define _MEMFUNCTIONS_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"

// Custom error codes
#define CHLE_MEM_ENOMEM     17000
#define CHLE_MEM_GEN        17001

// -------------------------------------------
// Functions exported

DllExpImp HRESULT CHL_MmAlloc(_Out_cap_(uSizeBytes) PVOID *ppvAddr, _In_ size_t uSizeBytes, _In_opt_ PDWORD pdwError);
DllExpImp void CHL_MmFree(_In_ PVOID *ppvToFree);

#ifdef __cplusplus
}
#endif

#endif // _MEMFUNCTIONS_H
