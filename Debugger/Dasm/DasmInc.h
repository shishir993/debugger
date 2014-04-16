
#ifndef _DASMINC_H
#define _DASMINC_H

#include "..\Inc\Common.h"

//#define UNIT_TESTS_ONLY

// input file types assigned to g_dwInputFileType
#define DASM_FTYPE_EXE	0x0001
#define DASM_FTYPE_DLL	0x0002

#define DASM_VERSION_STR		L"0.9"

#define MAX_CODE_PARTS	4
#define CODE_PART_CODE	0
#define CODE_PART_IAT	1

/*
 * Structures
 */
 typedef struct _BINFILEINFO {
	PIMAGE_NT_HEADERS pNTHeaders;	// pointer to NT headers when binary file mapped into memory
	DWORD dwVirtBaseOfCode;			// ImageBase + BaseOfCode
}BINFILEINFO;

// struct: store the location of an area within the .text section
// where there is no code (parts may be the IAT/...).
typedef struct _NonCodeLoc {
	INT iPartType;
	DWORD dwFilePointer;	// where in the file this part of code starts
	DWORD dwPartSize;		// size of this part of code
}NONCODE_LOC;

// struct: Includes an array of NONCODE_LOC struct to specify the various
// places in the .text/code section where there is no code but 
// other stuff like IAT.
typedef struct _NCodeLocs {
	HANDLE hFileBase;
	INT nNonCodeParts;
	NONCODE_LOC aNonCodeLocs[MAX_CODE_PARTS];
}NCODE_LOCS;

// Function comment skeleton
/* fGetPtrNTHeaders()
 *
 *
 * Args:
 *		
 *			
 * RetVal:
 *		
 */

#endif // _DASMINC_H
