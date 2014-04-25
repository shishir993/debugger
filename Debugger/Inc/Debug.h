
#ifndef _DEBUG_H
#define _DEBUG_H

#include "Common.h"
#include "UICreator.h"
#include "CHelpLibDll.h"
#include "MenuItems.h"
#include "Breakpoint.h"
#include "DebugCommon.h"

#define EXITCODE_TARGET_TERM    0xDEAD

// **** Functions ****
DWORD WINAPI dwDebugThreadEntry(LPVOID lpvArgs);


#endif // _DEBUG_H
