
#include "Inc\Assert.h"

void vAssert(const char* pszFile, UINT uLine)
{
    // ensure that all buffers are written out first
	fflush(NULL);
	fprintf(stderr, "Assertion failed in %s at Line %u\n", pszFile, uLine);
	fflush(stderr);

	exit(CE_ASSERT_FAILED);
}
