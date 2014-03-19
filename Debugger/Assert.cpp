
#include "Inc\Assert.h"

void vAssert(const char* pszFile, UINT uLine)
{
    // ensure that all buffers are written out first
	fflush(NULL);
	fprintf(stderr, "Assertion failed in %s at Line %u\n", pszFile, uLine);
	fflush(stderr);

    // Adding a hard breakpoint because:
    // In GUI programs, there is usually no console window open to display the assertion failure.
    // Even when a GUI program explicitly opens a console window, exiting the program causes
    // the console also to be closed immediately. So we cannot see where the assertion failed.

#ifdef _DEBUG       // just in case this code is called without using ASSERT macro
    __asm int 3        
#endif

	exit(CE_ASSERT_FAILED);
}
