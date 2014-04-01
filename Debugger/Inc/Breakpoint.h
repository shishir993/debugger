
#ifndef _BREAKPOINTS_H
#define _BREAKPOINTS_H

#include "Common.h"
#include "CHelpLibDll.h"
#include "StringLengths.h"

// ** Functions **
BOOL fBpInitialize(__out void **ppBreakpoints);
BOOL fBpTerminate(void *pBreakpoints);

#endif // _BREAKPOINTS_H
