// StringFunctions.h
// Contains functions that provide string operations
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      08/10/13 Initial version
//      09/09/14 Refactor to store defs in individual headers.
//

#ifndef _STRINGFUNCTIONS_H
#define _STRINGFUNCTIONS_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"

// -------------------------------------------
// Functions exported

DllExpImp PCWSTR CHL_SzGetFilenameFromPath(_In_z_ PCWSTR pszFilepath, _In_ int numCharsInput);

#ifdef __cplusplus
}
#endif

#endif // _STRINGFUNCTIONS_H
