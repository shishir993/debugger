
// IOFunctions.h
// Contains functions that provide IO operation services
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      06/23/13 Initial version
//      09/09/14 Refactor to store defs in individual headers.
//      09/12/14 Naming convention modifications
//

#ifndef _IOFUNCTIONS_H
#define _IOFUNCTIONS_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"
#include "MemFunctions.h"

// -------------------------------------------
// Functions exported

DllExpImp HRESULT CHL_IoReadLineFromStdin(_Inout_z_bytecap_x_(dwBufSize) PWSTR pszBuffer, _In_ DWORD dwBufSize);
DllExpImp HRESULT CHL_IoCreateFileWithSize(_Out_ PHANDLE phFile, _In_z_ PWCHAR pszFilepath, _In_ int iSizeBytes);


#ifdef __cplusplus
}
#endif

#endif // _IOFUNCTIONS_H
