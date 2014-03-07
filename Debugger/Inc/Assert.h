
#ifndef _ASSERT_H
#define _ASSERT_H

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "StringLengths.h"
#include "CustomErrors.h"

#ifdef _DEBUG
    void vAssert(const char* psFile, UINT uLine);
	#define ASSERT(x)	\
		if(x) {}		    \
		else			    \
			vAssert(__FILE__, __LINE__)
#else
	#define ASSERT(x)
#endif

#endif //_ASSERT_H
