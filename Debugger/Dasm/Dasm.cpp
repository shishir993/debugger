
#include "DASM.h"

// global variables
DWORD g_dwInputFileType;	// set by fBeginFileScan()
BINFILEINFO g_binFileInfo;

//// ** Controlling program options **
//// true by default
//BOOL g_fHeaders = TRUE;	// NTHeaders, FileHeader, Optional Header
//BOOL g_fDisasm = TRUE;
//
//// false by default
//BOOL g_fExports;
//BOOL g_fImports;

//int wmain(int argc, WCHAR **argv)
//{
//
//#ifdef UNIT_TESTS_ONLY
//	vRunTests();
//	//DEngine_FPUUnitTest();
//	//DEngine_MMXUnitTest();
//	//DEngine_SSEUnitTest();
//	return 0;
//#else
//
//	WCHAR wszFilepath[MAX_PATH+1] = L"";
//	WCHAR wszOutFile[MAX_PATH+1] = L"";
//
//	HANDLE hFile = NULL, hFileObj = NULL, hFileView = NULL;
//	IMAGE_NT_HEADERS *pNTHeaders = NULL;
//	DWORD dwCodeSecOffset = 0;
//	DWORD dwSizeOfCodeSection = 0;
//
//	NCODE_LOCS NNonCodeLocs;
//
//	//wprintf_s(L"%d %d %d\n", sizeof(long long), sizeof(INT), sizeof(DWORD));
//	//wprintf_s(L"sizeof(IMAGE_IMPORT_DESCRIPTOR) = %d\n", sizeof(IMAGE_IMPORT_DESCRIPTOR));
//
//	#ifdef NDEBUG
//	// Opening statement
//	wprintf(L"Shishir's Garage project::DASM v%s, a disassembler for 32bit PE files\
//				\nCoder: Shishir K Prasad {shishir993@yahoo.com}\
//				\nCopyright: None but don't forget to include my name as a reference in your code/webpage. :)\n\n", DASM_VERSION_STR);
//	#endif
//
//	/*
//	 * If no arguments are specified, then prompt user for path to
//	 * file to disassemble.
//	 * If one argument is given, then assume that as the file to be
//	 * disassembled.
//	 * If there are 2 arguments, then assume the first one as the 
//	 * exe/dll/... file, and the second one as the output file in
//	 * which we must write the output.
//	 *
//	 * todo: No support for output file yet!!
//	 */
//	//if(argc == 1)
//	//{
//	//	wprintf_s(L"Enter the path to file: ");
//	//	wscanf_s(L"%s", wszFilepath);
//	//}
//	//else if(argc == 2)
//	//{
//	//	// input file in argv[1]
//	//	wcscpy_s(wszFilepath, MAX_PATH+1, argv[1]);
//	//}
//	//else
//	//{
//	//	wprintf_s(L"usage: %s [<inputFile>]\n", argv[0]);
//	//	return 1;
//	//}
//
//	//if(!fCmdArgsHandler(argc, argv, wszFilepath, _countof(wszFilepath)))
//	//{
//	//	wprintf_s(L"main(): Unable to parse command line arguments\n");
//	//	wprintf_s(L"usage: %s\n\t[inputFile]\n\t[-exports|-e]\n\t[-imports|-i]\n\t[-headers|-h]\n\t[-disasm|-d]\n", argv[0]);
//	//	return 1;
//	//}
//
//	//wprintf_s(L"Using input file: %s\nOutput file: %s\n", wszFilepath, wszOutFile);
//	//wprintf_s(L"Using input file: %s\n", wszFilepath);
//
//	//if( ! fOpenAndMapFile(wszFilepath, &hFile, &hFileObj, &hFileView) )
//	//{
//	//	wprintf_s(L"Error opening input file\n");
//	//	return 1;
//	//}
//
//	wprintf_s(L"********************************\nDisassembling file: %s\n",
//				wszFilepath);
//
//	__try
//	{
//		// dump information from headers
//		if( ! fBeginFileScan(hFile, hFileObj, hFileView) )
//		{
//			wprintf_s(L"main(): fBeginFileScan() error\n");
//			return 1;
//		}
//
//		//if(g_fDisasm)
//		//{
//		//	// get pointer to code section
//		//	if( ! fGetPtrToCode((DWORD)hFileView, g_binFileInfo.pNTHeaders,
//		//						&dwCodeSecOffset, &dwSizeOfCodeSection, &g_binFileInfo.dwVirtBaseOfCode) )
//		//	{
//		//		wprintf_s(L"Cannot retrieve pointer to code section. Aborting...\n");
//		//		return 1;
//		//	}
//
//		//	//memset(&NNonCodeLocs, 0, sizeof(NNonCodeLocs));
//
//		//	// Check if the IAT is present within the .text section
//		//	// Doing this will lead to a clearer disassembly
//		//
//		//	//NNonCodeLocs.hFileBase = hFileView;
//
//		//	// Notes: Under construction
//		//	// begin disassembly
//		//	// fDisassembler(&NNonCodeLocs, g_binFileInfo.dwVirtBaseOfCode);
//
//		//	fDoDisassembly( (DWORD*)dwCodeSecOffset, dwSizeOfCodeSection, 
//		//					g_binFileInfo.dwVirtBaseOfCode );
//		//}// if(g_fDisasm)
//
//		return 0;
//	}
//	__finally
//	{
//		if(hFile != NULL && hFileObj != NULL && hFileView != NULL)
//			fCloseFile(hFile, hFileObj, hFileView);
//	}
//
//#endif	// UNIT_TESTS_ONLY
//
//}// wmain()


// Unit testing of individual functions
void vRunTests()
{
	
	// Util_vSplitModRMByte()
	BYTE bModRM;
	BYTE bMod;
	BYTE bReg;
	BYTE bRM;

	bModRM = 0x7c;	// 01 111 100
	bMod = bReg = bRM = 0xff;
	Util_vSplitModRMByte(bModRM, &bMod, &bReg, &bRM);
	if( bMod != 1 || bReg != 7 || bRM != 4)
		wprintf_s(L"!FAILED: Util_vSplitModRMByte(): %x, %x, %x, %x\n",
					bModRM, bMod, bReg, bRM);
	else
		wprintf_s(L"*Passed: Util_vSplitModRMByte(): %x, %x, %x, %x\n",
					bModRM, bMod, bReg, bRM);

	bModRM = 0xff;	// 11 111 111
	bMod = bReg = bRM = 0;
	Util_vSplitModRMByte(bModRM, &bMod, &bReg, &bRM);
	if( bMod != 3 || bReg != 7 || bRM != 7)
		wprintf_s(L"!FAILED: Util_vSplitModRMByte(): %x, %x, %x, %x\n",
					bModRM, bMod, bReg, bRM);
	else
		wprintf_s(L"*Passed: Util_vSplitModRMByte(): %x, %x, %x, %x\n",
					bModRM, bMod, bReg, bRM);

	// test Util_vGetDWBits()
	BYTE bD;
	BYTE bW;

	Util_vGetDWBits(0x00, &bD, &bW);
	if(bD != 0 || bW != 0)
		wprintf_s(L"!FAILED: Util_vGetDWBits(): %x, %x, %x\n", 0x00, bD, bW);
	else
		wprintf_s(L"*Passed: Util_vGetDWBits(): %x, %x, %x\n", 0x00, bD, bW);

	Util_vGetDWBits(0x03, &bD, &bW);
	if(bD != 1 || bW != 1)
		wprintf_s(L"!FAILED: Util_vGetDWBits(): %x, %x, %x\n", 0x03, bD, bW);
	else
		wprintf_s(L"*Passed: Util_vGetDWBits(): %x, %x, %x\n", 0x03, bD, bW);

	Util_vGetDWBits(0x01, &bD, &bW);
	if(bD != 0 || bW != 1)
		wprintf_s(L"!FAILED: Util_vGetDWBits(): %x, %x, %x\n", 0x01, bD, bW);
	else
		wprintf_s(L"*Passed: Util_vGetDWBits(): %x, %x, %x\n", 0x01, bD, bW);

	Util_vGetDWBits(0x02, &bD, &bW);
	if(bD != 1 || bW != 0)
		wprintf_s(L"!FAILED: Util_vGetDWBits(): %x, %x, %x\n", 0x02, bD, bW);
	else
		wprintf_s(L"*Passed: Util_vGetDWBits(): %x, %x, %x\n", 0x02, bD, bW);

	/*
	 * Util_vTwosComplementByte()
	 */
	BYTE ch = 0xe8;
	BYTE chCompl;	// 2's complement(ch)
	Util_vTwosComplementByte(ch, &chCompl);
	if(chCompl != 0x18)
		wprintf_s(L"!FAILED: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);

	ch = 0x00;
	Util_vTwosComplementByte(ch, &chCompl);
	if(chCompl != 0x00)
		wprintf_s(L"!FAILED: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);

	ch = 0x7f;
	Util_vTwosComplementByte(ch, &chCompl);
	if(chCompl != (BYTE)0x81)
		wprintf_s(L"!FAILED: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);

	ch = 0xff;
	Util_vTwosComplementByte(ch, &chCompl);
	if(chCompl != 0x01)
		wprintf_s(L"!FAILED: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementByte(): %x, %x\n", ch, chCompl);

	/*
	 * Util_vTwosComplementInt()
	 */
	INT i = 0x002b34e8;
	INT iCompl;	// 2's complement(ch)
	INT iTemp;

	iTemp = ((i ^ 0xffffffff) + 1);
	Util_vTwosComplementInt(i, &iCompl);
	if(iCompl != iTemp)
		wprintf_s(L"!FAILED: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);

	i = 0x00;
	iTemp = ((i ^ 0xffffffff) + 1);
	Util_vTwosComplementInt(i, &iCompl);
	if(iCompl != iTemp)
		wprintf_s(L"!FAILED: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);

	i = 0x7fffffff;
	iTemp = ((i ^ 0xffffffff) + 1);
	Util_vTwosComplementInt(i, &iCompl);
	if(iCompl != iTemp)
		wprintf_s(L"!FAILED: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);

	i = 0xffffffff;
	iTemp = ((i ^ 0xffffffff) + 1);
	Util_vTwosComplementInt(i, &iCompl);
	if(iCompl != iTemp)
		wprintf_s(L"!FAILED: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);

	i = 0x8b00401b;
	iTemp = ((i ^ 0xffffffff) + 1);
	Util_vTwosComplementInt(i, &iCompl);
	if(iCompl != iTemp)
		wprintf_s(L"!FAILED: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);

	i = 0x80000000;
	iTemp = ((i ^ 0xffffffff) + 1);
	Util_vTwosComplementInt(i, &iCompl);
	if(iCompl != iTemp)
		wprintf_s(L"!FAILED: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);
	else
		wprintf_s(L"*Passed: Util_vTwosComplementInt(): %x, %x\n", i, iCompl);

}// vRunTests()
