
// Assert.h
// Contains functions that provides the ASSERT() service
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      06/23/13 Initial version
//

/********************************************************************************************
 * ASSERT(x) is a debug-mode-only macro that is used to catch errors that must not
 * occur when software is released. It finds out bugs in the coding phase rather than
 * the testing phase.
 *
 * ASSERT accepts a relational expression as a macro-argument and tests whether it is
 * true. If it is true, nothing is done. If it is false, the user-defined _Assert()
 * function is called which prints the line number and filename in which the assertion
 * failed.
 *
 * Debug-mode-only is achieved using the #ifdef directive.
 * Code is compiled using Visual Studio 2010, which defines the macro _DEBUG in debug-mode.
 * In debug-mode _DEBUG is defined and in release-mode it is not defined.
 *******************************************************************************************/

#ifndef _ASSERT_H
#define _ASSERT_H

#ifdef __cplusplus
extern "C" {  
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
    void vAssert(const char* psFile, unsigned int uLine);
	#define ASSERT(x)	\
		if(x) {}		    \
		else			    \
			vAssert(__FILE__, __LINE__)
#else
	#define ASSERT(x)
#endif

#ifdef __cplusplus
}
#endif

#endif // _ASSERT_H
