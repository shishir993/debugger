
#include "DasmEngine.h"

#define MAGIC_DASMSTATE_STRUCT  0x00031009

// Lookup table for instruction names
WCHAR awszOpcodeLUT1[][MAX_OPCODE_NAME_LEN+1] = 
{
//		0			1			2			3			4			5			6			7		
//		8			9			A			B			C			D			E			F
/*0*/	L"add",		L"add",		L"add",		L"add",		L"add",		L"add",		L"push",	L"pop",	
/*0*/	L"or",		L"or",		L"or",		L"or",		L"or",		L"or",		L"push",	L"!twobyte",

/*1*/	L"adc",		L"adc",		L"adc",		L"adc",		L"adc",		L"adc",		L"push",	L"pop",	
/*1*/	L"sbb",		L"sbb",		L"sbb",		L"sbb",		L"sbb",		L"sbb",		L"push",	L"pop",	

/*2*/	L"and",		L"and",		L"and",		L"and",		L"and",		L"and",		L"sges",	L"daa",	
/*2*/	L"sub",		L"sub",		L"sub",		L"sub",		L"sub",		L"sub",		L"sgcs",	L"das",	

/*3*/	L"xor",		L"xor",		L"xor",		L"xor",		L"xor",		L"xor",		L"sgds",	L"aaa",
/*3*/	L"cmp",		L"cmp",		L"cmp",		L"cmp",		L"cmp",		L"cmp",		L"sgds",	L"aas",	

/*4*/	L"inc",		L"inc",		L"inc",		L"inc",		L"inc",		L"inc",		L"inc",		L"inc",	
/*4*/	L"dec",		L"dec",		L"dec",		L"dec",		L"dec",		L"dec",		L"dec",		L"dec",	

/*5*/	L"push",	L"push",	L"push",	L"push",	L"push",	L"push",	L"push",	L"push",
/*5*/	L"pop",		L"pop",		L"pop",		L"pop",		L"pop",		L"pop",		L"pop",		L"pop",	

//		0			1			2			3			4			5			6			7		
//		8			9			A			B			C			D			E			F
/*6*/	L"pushad",	L"popad",	L"bound",	L"arpl",	L"!sgfs",	L"!sggs",	L"!szop",	L"!szad",
/*6*/	L"push",	L"imul",	L"push",	L"imul",	L"ins",		L"ins",		L"outs",	L"outs",

/*7*/	L"jo",		L"jno",		L"jb",		L"jnb",		L"je",		L"jne",		L"jbe",		L"ja",	
/*7*/	L"js",		L"jns",		L"jpe",		L"jpo",		L"jl",		L"jge",		L"jle",		L"jg", 
	
/*8*/	L"",		L"",		L"",		L"",		L"test",	L"test",	L"xchg",	L"xchg",
/*8*/	L"mov",		L"mov",		L"mov",		L"mov",		L"mov",		L"lea",		L"mov",		L"pop", 

/*9*/	L"nop",		L"xchg",	L"xchg",	L"xchg",	L"xchg",	L"xchg",	L"xchg",	L"xchg",
/*9*/	L"cwd",		L"cdq",		L"callf",	L"wait",	L"pushfd",	L"popfd",	L"sahf",	L"lahf", 
	
/*A*/	L"mov",		L"mov",		L"mov",		L"mov",		L"movs",	L"movs",	L"cmps",	L"cmps",
/*A*/	L"test",	L"test",	L"stos",	L"stos",	L"lods",	L"lods",	L"scas",	L"scas",
	
/*B*/	L"mov",		L"mov",		L"mov",		L"mov",		L"mov",		L"mov",		L"mov",		L"mov",	
/*B*/	L"mov",		L"mov",		L"mov",		L"mov",		L"mov",		L"mov",		L"mov",		L"mov",	
	
/*C*/	L"!shift",	L"!shift",	L"ret",		L"ret",		L"les",		L"lds",		L"mov",		L"mov",	
/*C*/	L"enter",	L"leave",	L"retf",	L"retf",	L"int",		L"int",		L"into",	L"iretd",	
	
/*D*/	L"!shift",	L"!shift",	L"!shift",	L"!shift",	L"aam",		L"aad",		L"salc",	L"xlat",
/*D*/	L"!fpu",	L"!fpu",	L"!fpu",	L"!fpu",	L"!fpu",	L"!fpu",	L"!fpu",	L"!fpu", 
	
/*E*/	L"loopnz",	L"loopz",	L"loop",	L"jecxz",	L"in",		L"in",		L"out",		L"out",	
/*E*/	L"call",	L"jmp",		L"jmpf",	L"jmp",		L"in",		L"in",		L"out",		L"out", 
	
/*F*/	L"lock",	L"int1",	L"repne",	L"repe",	L"hlt",		L"cmc",		L"!",		L"!",	
/*F*/	L"clc",		L"stc",		L"cli",		L"sti",		L"cld",		L"std",		L"!inc",	L"!"
};

// Lookup table for instruction mnemonics(2byte opcodes)
WCHAR awszOpcodeLUT2[][MAX_OPCODE_NAME_LEN+1] = 
{
//		0				1			2			3			4				5			6			7		
//		8				9			A			B			C				D			E			F

/*0*/	L"!mult",		L"!mult",	L"lar",		L"lsl",		L"",			L"",		L"clts",	L"",
/*0*/	L"invd",		L"wbinvd",	L"",		L"ud2",		L"",			L"nop",		L"",		L"",

/*1*/	L"movups",		L"movups",	L"movlps",	L"movlps",	L"unpcklps",	L"unpckhps",L"movhps",	L"movhps",
/*1*/	L"!pfsse",		L"nop",		L"nop",		L"nop",		L"nop",			L"nop",		L"nop",		L"nop",

/*2*/	L"mov",			L"mov",		L"mov",		L"mov",		L"",			L"",		L"",		L"",
/*2*/	L"movaps",		L"movaps",	L"cvtpi2ps",L"movntps",	L"cvttps2pi",	L"cvtps2pi",L"ucomiss", L"comiss",

/*3*/	L"wrmsr",		L"rdtsc",	L"rdmsr",	L"rdpmc",	L"sysenter",	L"sysexit",	L"",		L"sse",
/*3*/	L"3byte",		L"",		L"3byte",	L"",		L"",			L"",		L"",		L"",

/*4*/	L"cmovo",		L"cmovno",	L"cmovb",	L"cmovae",	L"cmove",		L"cmovne",	L"cmovna",	L"cmova",
/*4*/	L"cmovs",		L"cmovns",	L"cmovp",	L"cmovpo",	L"cmovl",		L"cmovge",	L"cmovle",	L"cmovg",

/*5*/	L"movmskps",	L"sqrtps",	L"rsqrtps",	L"rcpps",	L"andps",		L"andnps",	L"orps",	L"xorps",
/*5*/	L"addps",		L"mulps",	L"cvtps2pd",L"cvtdq2ps",L"subps",		L"minps",	L"divps",	L"maxps",

/*6*/	L"punpcklbw",	L"punpcklwd",	L"punpckldq",	L"packsswb",		L"pcmpgtb",	L"pcmpgtw",	L"pcmpgtd",	L"packuswb",
/*6*/	L"punpckhbw",	L"punpckhwd",	L"punpckhdq",	L"packssdw",		L"mmx",		L"mmx",		L"movd",	L"movq",	

/*7*/	L"pshufw",		L"!mult",		L"!mult",		L"!mult",			L"pcmpeqb",	L"pcmpeqw",	L"pcmpeqd",	L"emms",
/*7*/	L"mmx",			L"mmx",			L"",			L"",				L"mmx",		L"mmx",		L"movd",	L"movq",

//		0				1			2			3			4			5			6			7		
//		8				9			A			B			C			D			E			F

/*8*/	L"jo",			L"jno",		L"jb",		L"jnb",		L"je",		L"jne",		L"jbe",		L"ja", 
/*8*/	L"js",			L"jns",		L"jpe",		L"jpo",		L"jl",		L"jge",		L"jle",		L"jg",

/*9*/	L"seto",		L"setno",	L"setb",	L"setnb",	L"sete",	L"setne",	L"setbe",	L"seta", 
/*9*/	L"sets",		L"setns",	L"setpe",	L"setpo",	L"setl",	L"jge",		L"setle",	L"setg",

/*A*/	L"push",		L"pop",		L"cpuid",	L"bt",		L"shld",	L"shld",	L"",		L"", 
/*A*/	L"push",		L"pop",		L"rsm",		L"bts",		L"shrd",	L"shrd",	L"!mult",	L"imul",

/*B*/	L"cmpxchg",		L"cmpxchg",	L"lss",		L"btr",		L"lfs",		L"lgs",		L"movzx",	L"movzx",
/*B*/	L"popcnt",		L"ud",		L"!btx",	L"btc",		L"bsf",		L"bsr",		L"movsx",	L"movsx",

/*C*/	L"xadd",		L"xadd",	L"cmpps",	L"sse",		L"pinsrw",	L"pextrw",	L"shufps",	L"cmpxchg8b",
/*C*/	L"bswap",		L"bswap",	L"bswap",	L"bswap",	L"bswap",	L"bswap",	L"bswap",	L"bswap", 

/*D*/	L"sse",			L"psrlw",	L"psrld",	L"psrlq",	L"sse",		L"pmullw",	L"sse",		L"pmovmskb",
/*D*/	L"psubusb",		L"psubusw",	L"pminub",	L"pand",	L"paddusb",	L"paddusw",	L"pmaxub",	L"pandn",

/*E*/	L"pavgb",		L"psraw",	L"psrad",	L"pavgw",	L"pmulhuw",	L"pmulhuw",	L"cvttpd2dq",L"movntq",
/*E*/	L"psubsb",		L"psubsw",	L"pminsw",	L"por",		L"paddsb",	L"paddsw",	L"pmaxsw",	L"pxor",

/*F*/	L"sse",			L"psllw",	L"pslld",	L"psllq",	L"sse",		L"pmaddwd",	L"psadbw",	L"maskmovq",
/*F*/	L"psubb",		L"psubw",	L"psubd",	L"mmx",		L"paddb",	L"paddw",	L"paddd",	L"!",

};

// ** FPU Instructions **
WCHAR awszFPUOpcodeLUTRegEx[][MAX_OPCODE_NAME_LEN+1] = // Instructions that use only the 'reg' field as opcode extension
{
//			0			1			2			3			4			5			6			7
/*D8,0*/	L"fadd",	L"fmul",	L"fcom",	L"fcomp",	L"fsub",	L"fsubr",	L"fdiv",	L"fdivr",
/*D9,1*/	L"fld",		L"!",		L"fst",		L"fstp",	L"fldenv",	L"fldcw",	L"fnstenv",	L"fnstcw",
/*DA,2*/	L"fiadd",	L"fimul",	L"ficom",	L"ficomp",	L"fisub",	L"fisubr",	L"fidiv",	L"fidivr",
/*DB,3*/	L"fild",	L"!",		L"fist",	L"fistp",	L"!",		L"fld",		L"!",		L"fstp",
/*DC,4*/	L"fadd",	L"fmul",	L"fcom",	L"fcomp",	L"fsub",	L"fsubr",	L"fdiv",	L"fdivr",
/*DD,5*/	L"fld",		L"!",		L"fst",		L"fstp",	L"frstor",	L"!",		L"fnsave",	L"fnstsw",
/*DE,6*/	L"fiadd",	L"fimul",	L"ficom",	L"ficomp",	L"fisub",	L"fisubr",	L"fidiv",	L"fidivr",
/*DF,7*/	L"fild",	L"!",		L"fist",	L"fistp",	L"fbld",	L"fild",	L"fbstp",	L"fistp"
};

WCHAR awszFPUOpcodeLUTFullEx[][MAX_OPCODE_NAME_LEN+1] = // Instructions that use full ModRM byte as opcode extension
{
//			0			1			2			3			4			5			6			7
 //			8			9			A			B			C			D			E			F
 /*D8,C0*/	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",
 /*D8,C8*/	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",
 /*D8,D0*/	L"fcom",	L"fcom",	L"fcom",	L"fcom",	L"fcom",	L"fcom",	L"fcom",	L"fcom",
 /*D8,D8*/	L"fcomp",	L"fcomp",	L"fcomp",	L"fcomp",	L"fcomp",	L"fcomp",	L"fcomp",	L"fcomp",
 /*D8,E0*/	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",
 /*D8,E8*/	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",
 /*D8,F0*/	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",
 /*D8,F8*/	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",

 /*D9,C0*/	L"fld",		L"fld",		L"fld",		L"fld",		L"fld",		L"fld",		L"fld",		L"fld",	
 /*D9,C8*/	L"fxch",	L"fxch",	L"fxch",	L"fxch",	L"fxch",	L"fxch",	L"fxch",	L"fxch",
 /*D9,D0*/	L"fnop",	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*D9,D8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*D9,E0*/	L"fchs",	L"fabs",	L"!",		L"!",		L"ftst",	L"fxam",	L"!",		L"!",
 /*D9,E8*/	L"fld1",	L"fldl2t",	L"fldl2e",	L"fldpi",	L"fldlg2",	L"fldln2",	L"fldz",	L"!",
 /*D9,F0*/	L"f2xm1",	L"fyl2x",	L"fptan",	L"fpatan",	L"fxtract",	L"fprem1",	L"fdecstp",	L"fincstp",
 /*D9,F8*/	L"fprem",	L"fyl2xp1",	L"fsqrt",	L"fsincos",	L"frndint",	L"fscale",	L"fsin",	L"fcos",

 /*DA,C0*/	L"fcmovb",	L"fcmovb",	L"fcmovb",	L"fcmovb",	L"fcmovb",	L"fcmovb",	L"fcmovb",	L"fcmovb",
 /*DA,C8*/	L"fcmove",	L"fcmove",	L"fcmove",	L"fcmove",	L"fcmove",	L"fcmove",	L"fcmove",	L"fcmove",
 /*DA,D0*/	L"fcmovbe",	L"fcmovbe",	L"fcmovbe",	L"fcmovbe",	L"fcmovbe",	L"fcmovbe",	L"fcmovbe",	L"fcmovbe",
 /*DA,D8*/	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",
 /*DA,E0*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DA,E8*/	L"!",		L"fucompp",	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DA,F0*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DA,F8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",

 /*DB,C0*/	L"fcmovnb",	L"fcmovnb",	L"fcmovnb",	L"fcmovnb",	L"fcmovnb",	L"fcmovnb",	L"fcmovnb",	L"fcmovnb",	
 /*DB,C8*/	L"fcmovne",	L"fcmovne",	L"fcmovne",	L"fcmovne",	L"fcmovne",	L"fcmovne",	L"fcmovne",	L"fcmovne",
 /*DB,D0*/	L"fcmovnbe",L"fcmovnbe",L"fcmovnbe",L"fcmovnbe",L"fcmovnbe",L"fcmovnbe",L"fcmovnbe",L"fcmovnbe",
 /*DB,D8*/	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",	L"fcmovu",
 /*DB,E0*/	L"!",		L"!",		L"fnclex",	L"fninit",	L"!",		L"!",		L"!",		L"!",
 /*DB,E8*/	L"fucomi",	L"fucomi",	L"fucomi",	L"fucomi",	L"fucomi",	L"fucomi",	L"fucomi",	L"fucomi",
 /*DB,F0*/	L"fcomi",	L"fcomi",	L"fcomi",	L"fcomi",	L"fcomi",	L"fcomi",	L"fcomi",	L"fcomi",
 /*DB,F8*/	L"!",		L"!",		L"fnclex",	L"fninit",	L"!",		L"!",		L"!",		L"!",

 /*DC,C0*/	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",	L"fadd",
 /*DC,C8*/	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",	L"fmul",
 /*DC,D0*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DC,D8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DC,E0*/	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",	L"fsubr",
 /*DC,E8*/	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",	L"fsub",
 /*DC,F0*/	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",	L"fdivr",
 /*DC,F8*/	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",	L"fdiv",

 /*DD,C0*/	L"ffree",	L"ffree",	L"ffree",	L"ffree",	L"ffree",	L"ffree",	L"ffree",	L"ffree",
 /*DD,C8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DD,D0*/	L"fst",		L"fst",		L"fst",		L"fst",		L"fst",		L"fst",		L"fst",		L"fst",	
 /*DD,D8*/	L"fstp",	L"fstp",	L"fstp",	L"fstp",	L"fstp",	L"fstp",	L"fstp",	L"fstp",
 /*DD,E0*/	L"fucom",	L"fucom",	L"fucom",	L"fucom",	L"fucom",	L"fucom",	L"fucom",	L"fucom",
 /*DD,E8*/	L"fucomp",	L"fucomp",	L"fucomp",	L"fucomp",	L"fucomp",	L"fucomp",	L"fucomp",	L"fucomp",
 /*DD,F0*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DD,F8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",

 /*DE,C0*/	L"faddp",	L"faddp",	L"faddp",	L"faddp",	L"faddp",	L"faddp",	L"faddp",	L"faddp",
 /*DE,C8*/	L"fmulp",	L"fmulp",	L"fmulp",	L"fmulp",	L"fmulp",	L"fmulp",	L"fmulp",	L"fmulp",
 /*DE,D0*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DE,D8*/	L"!",		L"fcompp",	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DE,E0*/	L"fsubrp",	L"fsubrp",	L"fsubrp",	L"fsubrp",	L"fsubrp",	L"fsubrp",	L"fsubrp",	L"fsubrp",
 /*DE,E8*/	L"fsubp",	L"fsubp",	L"fsubp",	L"fsubp",	L"fsubp",	L"fsubp",	L"fsubp",	L"fsubp",
 /*DE,F0*/	L"fdivrp",	L"fdivrp",	L"fdivrp",	L"fdivrp",	L"fdivrp",	L"fdivrp",	L"fdivrp",	L"fdivrp",
 /*DE,F8*/	L"fdivp",	L"fdivp",	L"fdivp",	L"fdivp",	L"fdivp",	L"fdivp",	L"fdivp",	L"fdivp",

 /*DF,C0*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DF,C8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DF,D0*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DF,D8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DF,E0*/	L"fnstsw",	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
 /*DF,E8*/	L"fucomip",	L"fucomip",	L"fucomip",	L"fucomip",	L"fucomip",	L"fucomip",	L"fucomip",	L"fucomip",
 /*DF,F0*/	L"fcomip",	L"fcomip",	L"fcomip",	L"fcomip",	L"fcomip",	L"fcomip",	L"fcomip",	L"fcomip",
 /*DF,F8*/	L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",		L"!",
};


// Array of function pointers to opcode handler functions.
// These are arranged according to the opcodes. Indexing is done
// this way: Opcode 0-3bits selects row and 4-7bits selects the 
// column.
// All opcode handlers take one BYTE argument(the opcode) 
// and return a BOOL value.
BOOL (*afpOpcHndlrs[16][16])(PDASMSTATE, BYTE) = 
{
//		0						1						2						3						4
//		5						6						7						8						9
//		A						B						C						D						E
//		F
/*0*/	OPCHndlrALU_ADD,		OPCHndlrALU_ADD,		OPCHndlrALU_ADD,		OPCHndlrALU_ADD,		OPCHndlrALU_ADD,
/*0*/	OPCHndlrALU_ADD,		OPCHndlrStack_PUSH,		OPCHndlrStack_POP,		OPCHndlrALU_OR,			OPCHndlrALU_OR,
/*0*/	OPCHndlrALU_OR,			OPCHndlrALU_OR,			OPCHndlrALU_OR,			OPCHndlrALU_OR,			OPCHndlrStack_PUSH,
/*0*/	OPCHndlr_2ByteHandler,

/*1*/	OPCHndlrALU_ADD,		OPCHndlrALU_ADD,		OPCHndlrALU_ADD,		OPCHndlrALU_ADD,		OPCHndlrALU_ADD,
/*1*/	OPCHndlrALU_ADD,		OPCHndlrStack_PUSH,		OPCHndlrStack_POP,		OPCHndlrALU_SUB,		OPCHndlrALU_SUB,
/*1*/	OPCHndlrALU_SUB,		OPCHndlrALU_SUB,		OPCHndlrALU_SUB,		OPCHndlrALU_SUB,		OPCHndlrStack_PUSH,
/*1*/	OPCHndlrStack_POP,

/*2*/	OPCHndlrALU_AND,		OPCHndlrALU_AND,		OPCHndlrALU_AND,		OPCHndlrALU_AND,		OPCHndlrALU_AND,
/*2*/	OPCHndlrALU_AND,		OPCHndlrPrefix_Ovride,	OPCHndlrALU_DAA,		OPCHndlrALU_SUB,		OPCHndlrALU_SUB,
/*2*/	OPCHndlrALU_SUB,		OPCHndlrALU_SUB,		OPCHndlrALU_SUB,		OPCHndlrALU_SUB,		OPCHndlrPrefix_Ovride,	
/*2*/	OPCHndlrALU_DAS,

/*3*/	OPCHndlrALU_XOR,		OPCHndlrALU_XOR,		OPCHndlrALU_XOR,		OPCHndlrALU_XOR,		OPCHndlrALU_XOR,
/*3*/	OPCHndlrALU_XOR,		OPCHndlrPrefix_Ovride,	OPCHndlrALU_AAA,		OPCHndlrCC_CMP,			OPCHndlrCC_CMP,
/*3*/	OPCHndlrCC_CMP,			OPCHndlrCC_CMP,			OPCHndlrCC_CMP,			OPCHndlrCC_CMP,			OPCHndlrPrefix_Ovride,
/*3*/	OPCHndlrALU_AAS,

/*4*/	OPCHndlrALU_INC,		OPCHndlrALU_INC,		OPCHndlrALU_INC,		OPCHndlrALU_INC,		OPCHndlrALU_INC,
/*4*/	OPCHndlrALU_INC,		OPCHndlrALU_INC,		OPCHndlrALU_INC,		OPCHndlrALU_DEC,		OPCHndlrALU_DEC,
/*4*/	OPCHndlrALU_DEC,		OPCHndlrALU_DEC,		OPCHndlrALU_DEC,		OPCHndlrALU_DEC,		OPCHndlrALU_DEC,
/*4*/	OPCHndlrALU_DEC,

/*5*/	OPCHndlrStack_PUSH,		OPCHndlrStack_PUSH,		OPCHndlrStack_PUSH,		OPCHndlrStack_PUSH,		OPCHndlrStack_PUSH,
/*5*/	OPCHndlrStack_PUSH,		OPCHndlrStack_PUSH,		OPCHndlrStack_PUSH,		OPCHndlrStack_POP,		OPCHndlrStack_POP,
/*5*/	OPCHndlrStack_POP,		OPCHndlrStack_POP,		OPCHndlrStack_POP,		OPCHndlrStack_POP,		OPCHndlrStack_POP,
/*5*/	OPCHndlrStack_POP,

/*6*/	OPCHndlrStack_PUSHxx,	OPCHndlrStack_POPxx,	OPCHndlrCC_BOUND,		OPCHndlrCC_ARPL,		OPCHndlrPrefix_Ovride,
/*6*/	OPCHndlrPrefix_Ovride,	OPCHndlrPrefix_Ovride,	OPCHndlrPrefix_Ovride,	OPCHndlrStack_PUSH,		OPCHndlrALU_MUL,
/*6*/	OPCHndlrStack_PUSH,		OPCHndlrALU_MUL,		OPCHndlrSysIO_INS,		OPCHndlrSysIO_INS,		OPCHndlrSysIO_OUTS,
/*6*/	OPCHndlrSysIO_OUTS,

/*7*/	OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,
/*7*/	OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,
/*7*/	OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,
/*7*/	OPCHndlrCC_JUMP,

//		0						1						2						3						4
//		5						6						7						8						9
//		A						B						C						D						E
//		F
/*8*/	OPCHndlrMulti_8x,		OPCHndlrMulti_8x,		OPCHndlrMulti_8x,		OPCHndlrMulti_8x,		OPCHndlrCC_TEST,
/*8*/	OPCHndlrCC_TEST,		OPCHndlrMem_XCHG,		OPCHndlrMem_XCHG,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,
/*8*/	OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_LEA,		OPCHndlrMem_MOV,	
/*8*/	OPCHndlrStack_POP,

/*9*/	OPCHndlr_NOP,			OPCHndlrMem_XCHG,		OPCHndlrMem_XCHG,		OPCHndlrMem_XCHG,		OPCHndlrMem_XCHG,
/*9*/	OPCHndlrMem_XCHG,		OPCHndlrMem_XCHG,		OPCHndlrMem_XCHG,		OPCHndlrMem_CWDE,		OPCHndlrMem_CDQ,
/*9*/	OPCHndlrCC_CALL,		OPCHndlrSysIO_WAIT,		OPCHndlrStack_PUSHxx,	OPCHndlrStack_POPxx,	OPCHndlrMem_SAHF,
/*9*/	OPCHndlrMem_LAHF,

/*A*/	OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOVS,
/*A*/	OPCHndlrMem_MOVS,		OPCHndlrCC_CMPS,		OPCHndlrCC_CMPS,		OPCHndlrCC_TEST,		OPCHndlrCC_TEST,
/*A*/	OPCHndlrMem_STOS,		OPCHndlrMem_STOS,		OPCHndlrMem_LODS,		OPCHndlrMem_LODS,		OPCHndlrCC_SCAS,
/*A*/	OPCHndlrCC_SCAS,

/*B*/	OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,
/*B*/	OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,
/*B*/	OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,
/*B*/	OPCHndlrMem_MOV,

/*C*/	OPCHndlrALU_Shift,		OPCHndlrALU_Shift,		OPCHndlrCC_RETN,		OPCHndlrCC_RETN,		OPCHndlrMem_LES,
/*C*/	OPCHndlrMem_LDS,		OPCHndlrMem_MOV,		OPCHndlrMem_MOV,		OPCHndlrStack_ENTER,	OPCHndlrStack_LEAVE,
/*C*/	OPCHndlrCC_RETN,		OPCHndlrCC_RETN,		OPCHndlrSysIO_INT,		OPCHndlrSysIO_INT,		OPCHndlrSysIO_INT,
/*C*/	OPCHndlrCC_IRETD,

/*D*/	OPCHndlrALU_Shift,		OPCHndlrALU_Shift,		OPCHndlrALU_Shift,		OPCHndlrALU_Shift,		OPCHndlrALU_AAM,
/*D*/	OPCHndlrALU_AAD,		OPCHndlrALU_SALC,		OPCHndlrMem_XLAT,		OPCHndlrFPU_All,		OPCHndlrFPU_All,
/*D*/	OPCHndlrFPU_All,		OPCHndlrFPU_All,		OPCHndlrFPU_All,		OPCHndlrFPU_All,		OPCHndlrFPU_All,
/*D*/	OPCHndlrFPU_All,

/*E*/	OPCHndlrCC_CLOOP,		OPCHndlrCC_CLOOP,		OPCHndlrCC_CLOOP,		OPCHndlrCC_JUMP,		OPCHndlrSysIO_IN,
/*E*/	OPCHndlrSysIO_IN,		OPCHndlrSysIO_OUT,		OPCHndlrSysIO_OUT,		OPCHndlrCC_CALL,		OPCHndlrCC_JUMP,
/*E*/	OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrSysIO_INDX,		OPCHndlrSysIO_INDX,		OPCHndlrSysIO_OUTDX,
/*E*/	OPCHndlrSysIO_OUTDX,

/*F*/	OPCHndlrPrefix_LOCK,	OPCHndlrSysIO_IceBP,	OPCHndlrPrefix_CREP,	OPCHndlrPrefix_CREP,	OPCHndlrSysIO_HLT,
/*F*/	OPCHndlrCC_EFLAGS,		OPCHndlrMulti_fx,		OPCHndlrMulti_fx,		OPCHndlrCC_EFLAGS,		OPCHndlrCC_EFLAGS,
/*F*/	OPCHndlrCC_EFLAGS,		OPCHndlrCC_EFLAGS,		OPCHndlrCC_EFLAGS,		OPCHndlrCC_EFLAGS,		OPCHndlrMulti_IncDec,
/*F*/	OPCHndlrMulti_FF

}; // (*afpOpcHndlrs[][])()

// Array of function pointers to 2byte opcode handlers
// These are arranged according to the opcodes. Indexing is done
// this way: Opcode(Byte2) 4-7bits selects row and 0-3bits selects the 
// column.
// All opcode handlers take one BYTE argument(the opcode) 
// and return a BOOL value.
BOOL (*afpOpcHndlrs2[16][16])(PDASMSTATE, BYTE) = 
{
//		0						1						2						3						4
//		5						6						7						8						9
//		A						B						C						D						E
//		F
/*0*/	OPCHndlrSysIO_LdtTrS,	OPCHndlrSysIO_GdtIdMsw,	OPCHndlrMem_LAR,		OPCHndlrMem_LSL,		OPCHndlr_INVALID,
/*0*/	OPCHndlr_INVALID,		OPCHndlrSysIO_CLTS,		OPCHndlr_INVALID,		OPCHndlrSysIO_INVD,		OPCHndlrSysIO_WBINVD,
/*0*/	OPCHndlr_INVALID,		OPCHndlrSysIO_UD2,		OPCHndlr_INVALID,		OPCHndlr_NOP,			OPCHndlr_INVALID,
/*0*/	OPCHndlr_INVALID,

/*1*/	OPCHndlrSSE_Mov,		OPCHndlrSSE_Mov,		OPCHndlrSSE_Mov,		OPCHndlrSSE_Mov,		OPCHndlrSSE_Conv,
/*1*/	OPCHndlrSSE_Conv,		OPCHndlrSSE_Mov,		OPCHndlrSSE_Mov,		OPCHndlrSSE_Prefetch18,	OPCHndlr_HNOP,
/*1*/	OPCHndlr_HNOP,			OPCHndlr_HNOP,			OPCHndlr_HNOP,			OPCHndlr_HNOP,			OPCHndlr_HNOP,
/*1*/	OPCHndlr_HNOP,

/*2*/	OPCHndlrMem_MOVCrDr,	OPCHndlrMem_MOVCrDr,	OPCHndlrMem_MOVCrDr,	OPCHndlrMem_MOVCrDr,	OPCHndlr_INVALID,
/*2*/	OPCHndlr_INVALID,		OPCHndlr_INVALID,		OPCHndlr_INVALID,		OPCHndlrSSE_Mov,		OPCHndlrSSE_Mov,
/*2*/	OPCHndlrSSE_Conv,		OPCHndlrSSE_Mov,		OPCHndlrSSE_Conv,		OPCHndlrSSE_Conv,		OPCHndlrSSE_Cmp,
/*2*/	OPCHndlrSSE_Cmp,

/*3*/	OPCHndlrSysIO_WRMSR,	OPCHndlrSysIO_RDTSC,	OPCHndlrSysIO_RDMSR,	OPCHndlrSysIO_RDPMC,	OPCHndlrSysIO_SYSENTER,
/*3*/	OPCHndlrSysIO_SYSEXIT,	OPCHndlr_INVALID,		OPCHndlrUnKwn_,			OPCHndlr_3ByteHandler,	OPCHndlr_INVALID,
/*3*/	OPCHndlr_3ByteHandler,	OPCHndlr_INVALID,		OPCHndlr_INVALID,		OPCHndlr_INVALID,		OPCHndlr_INVALID,
/*3*/	OPCHndlr_INVALID,

/*4*/	OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,
/*4*/	OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,
/*4*/	OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,		OPCHndlrCC_CMOV,
/*4*/	OPCHndlrCC_CMOV,

/*5*/	OPCHndlrSSE_Mov,		OPCHndlrSSE_Arith,		OPCHndlrSSE_Arith,		OPCHndlrSSE_Arith,		OPCHndlrSSE_Logical,
/*5*/	OPCHndlrSSE_Logical,	OPCHndlrSSE_Logical,	OPCHndlrSSE_Logical,	OPCHndlrSSE_Arith,		OPCHndlrSSE_Arith,
/*5*/	OPCHndlrSSE_Conv,		OPCHndlrSSE_Conv,		OPCHndlrSSE_Arith,		OPCHndlrSSE_Logical,	OPCHndlrSSE_Arith,
/*5*/	OPCHndlrSSE_Logical,

/*6*/	OPCHndlrMMX_PConv,		OPCHndlrMMX_PConv,		OPCHndlrMMX_PConv,		OPCHndlrMMX_PConv,		OPCHndlrMMX_PCmp,
/*6*/	OPCHndlrMMX_PCmp,		OPCHndlrMMX_PCmp,		OPCHndlrMMX_PConv,		OPCHndlrMMX_PConv,		OPCHndlrMMX_PConv,
/*6*/	OPCHndlrMMX_PConv,		OPCHndlrMMX_PConv,		OPCHndlrUnKwn_MMX,		OPCHndlrUnKwn_MMX,		OPCHndlrMMX_PMov,
/*6*/	OPCHndlrMMX_PMov,

/*7*/	OPCHndlrMMX_PSHUFW,		OPCHndlrMMX_PMulti7x,	OPCHndlrMMX_PMulti7x,	OPCHndlrMMX_PMulti7x,	OPCHndlrMMX_PCmp,
/*7*/	OPCHndlrMMX_PCmp,		OPCHndlrMMX_PCmp,		OPCHndlrMMX_EMMS,		OPCHndlrUnKwn_MMX,		OPCHndlrUnKwn_MMX,
/*7*/	OPCHndlr_INVALID,		OPCHndlr_INVALID,		OPCHndlrUnKwn_SSE,		OPCHndlrUnKwn_SSE,		OPCHndlrMMX_PMov,
/*7*/	OPCHndlrMMX_PMov,

//		0						1						2						3						4
//		5						6						7						8						9
//		A						B						C						D						E
//		F
/*8*/	OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,
/*8*/	OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,
/*8*/	OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,		OPCHndlrCC_JUMP,
/*8*/	OPCHndlrCC_JUMP,

/*9*/	OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,
/*9*/	OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,
/*9*/	OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,		OPCHndlrCC_SETxx,
/*9*/	OPCHndlrCC_SETxx,

/*A*/	OPCHndlrStack_PUSH,		OPCHndlrStack_POP,		OPCHndlrSysIO_CPUID,	OPCHndlrMem_BT,			OPCHndlrALU_Shift,
/*A*/	OPCHndlrALU_Shift,		OPCHndlr_INVALID,		OPCHndlr_INVALID,		OPCHndlrStack_PUSH,		OPCHndlrStack_POP,
/*A*/	OPCHndlrSysIO_RSM,		OPCHndlrMem_BTS,		OPCHndlrALU_Shift,		OPCHndlrALU_Shift,		OPCHndlrSSE_MultiAE,
/*A*/	OPCHndlrALU_MUL,

/*B*/	OPCHndlrCC_CmpXchg,		OPCHndlrCC_CmpXchg,		OPCHndlrMem_LSS,		OPCHndlrMem_BTR,		OPCHndlrMem_LFS,
/*B*/	OPCHndlrMem_LGS,		OPCHndlrMem_MOVZX,		OPCHndlrMem_MOVZX,		OPCHndlrUnKwn_,			OPCHndlrSysIO_UD2,
/*B*/	OPCHndlrMulti_BitTestX,	OPCHndlrMem_BTC,		OPCHndlrMem_BSF,		OPCHndlrMem_BSR,		OPCHndlrMem_MOVSX,
/*B*/	OPCHndlrMem_MOVSX,

/*C*/	OPCHndlrALU_XADD,		OPCHndlrALU_XADD,		OPCHndlrSSE_Cmp,		OPCHndlrUnKwn_SSE,		OPCHndlrMMX_PINSRW,
/*C*/	OPCHndlrMMX_PEXTRW,		OPCHndlrSSE_SHUFPS,		OPCHndlrCC_CmpXchg,		OPCHndlrMem_ByteSWAP,	OPCHndlrMem_ByteSWAP,
/*C*/	OPCHndlrMem_ByteSWAP,	OPCHndlrMem_ByteSWAP,	OPCHndlrMem_ByteSWAP,	OPCHndlrMem_ByteSWAP,	OPCHndlrMem_ByteSWAP,
/*C*/	OPCHndlrMem_ByteSWAP,

/*D*/	OPCHndlrUnKwn_SSE,		OPCHndlrMMX_PShift,		OPCHndlrMMX_PShift,		OPCHndlrMMX_PShift,		OPCHndlrUnKwn_MMX,
/*D*/	OPCHndlrMMX_PArith,		OPCHndlrUnKwn_SSE,		OPCHndlrMMX_PMOVMSKB,	OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,
/*D*/	OPCHndlrMMX_PMaxMinAvg,	OPCHndlrMMX_PLogical,	OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,		OPCHndlrMMX_PMaxMinAvg,
/*D*/	OPCHndlrMMX_PLogical,

/*E*/	OPCHndlrMMX_PMaxMinAvg,	OPCHndlrMMX_PShift,		OPCHndlrMMX_PShift,		OPCHndlrMMX_PMaxMinAvg,	OPCHndlrMMX_PArith,
/*E*/	OPCHndlrMMX_PArith,		OPCHndlrSSE_Conv,		OPCHndlrSSE_Mov,		OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,
/*E*/	OPCHndlrMMX_PMaxMinAvg,	OPCHndlrMMX_PLogical,	OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,		OPCHndlrMMX_PMaxMinAvg,
/*E*/	OPCHndlrMMX_PLogical,

/*F*/	OPCHndlrUnKwn_SSE,		OPCHndlrMMX_PShift,		OPCHndlrMMX_PShift,		OPCHndlrMMX_PShift,		OPCHndlrUnKwn_MMX,
/*F*/	OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,		OPCHndlrSSE_Mov,		OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,
/*F*/	OPCHndlrMMX_PArith,		OPCHndlrUnKwn_SSE,		OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,		OPCHndlrMMX_PArith,
/*F*/	OPCHndlrUnKwn_SSE

}; // (*afpOpcHndlrs2[][])()

/***************************************************
 *	FPU Instructions *
 **************************************************/

 // LookUpTable for instructions whose ModRM's value lies
 // between 00h and BFh. These instructions use only the
 // 'reg' field of the ModRM byte as the opcode extension.
 BOOL (*afpFPUModRMRegEx[8][8])(PDASMSTATE, BYTE) = 
 {
 /*D8,0*/	OPCHndlrFPU_FADD,	OPCHndlrFPU_FMUL,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
			OPCHndlrFPU_FSUB,	OPCHndlrFPU_FSUB,	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,
 
 /*D9,1*/	OPCHndlrFPU_FLoad,	OPCHndlrFPU_Invalid,OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		
			OPCHndlrFPU_CtlOp,	OPCHndlrFPU_CtlOp,	OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,
 
 /*DA,2*/	OPCHndlrFPU_FADD,	OPCHndlrFPU_FMUL,	OPCHndlrFPU_FCmpInts,	OPCHndlrFPU_FCmpInts,
			OPCHndlrFPU_FSUB,	OPCHndlrFPU_FSUB,	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,
 
 /*DB,3*/	OPCHndlrFPU_FLoad,	OPCHndlrFPU_Invalid,OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,
			OPCHndlrFPU_Invalid,OPCHndlrFPU_FLoad,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_FStore,
 
 
 /*DC,4*/	OPCHndlrFPU_FADD,	OPCHndlrFPU_FMUL,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
			OPCHndlrFPU_FSUB,	OPCHndlrFPU_FSUB,	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,
 
 /*DD,5*/	OPCHndlrFPU_FLoad,	OPCHndlrFPU_Invalid,OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,
			OPCHndlrFPU_CtlOp,	OPCHndlrFPU_Invalid,OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,
 
 /*DE,6*/	OPCHndlrFPU_FADD,	OPCHndlrFPU_FMUL,	OPCHndlrFPU_FCmpInts,	OPCHndlrFPU_FCmpInts,
			OPCHndlrFPU_FSUB,	OPCHndlrFPU_FSUB,	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,
 
 /*DF,7*/	OPCHndlrFPU_FLoad,	OPCHndlrFPU_Invalid,OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,
			OPCHndlrFPU_FLoad,	OPCHndlrFPU_FLoad,	OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,
 };

 // LookUpTable for instructions whose ModRM's value lies
 // between C0h and FFh. These instructions use the full
 // ModRM byte as the opcode extension.
 // #possible ModRM values between C0h and FFh = FFh - C0h + 1h = 40h
 BOOL (*afpFPUModRMFullEx[8][0x40])(PDASMSTATE, BYTE) = 
 {
 //			0						1						2						3						4						5						6						7
 //			8						9						A						B						C						D						E						F
 /*D8,C0*/	OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,	
 /*D8,C8*/	OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,
 /*D8,D0*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*D8,D8*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*D8,E0*/	OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,
 /*D8,E8*/	OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,
 /*D8,F0*/	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,
 /*D8,F8*/	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,

 /*D9,C0*/	OPCHndlrFPU_FLoad,		OPCHndlrFPU_FLoad,		OPCHndlrFPU_FLoad,		OPCHndlrFPU_FLoad,		OPCHndlrFPU_FLoad,		OPCHndlrFPU_FLoad,		OPCHndlrFPU_FLoad,		OPCHndlrFPU_FLoad,
 /*D9,C8*/	OPCHndlrFPU_FXCH,		OPCHndlrFPU_FXCH,		OPCHndlrFPU_FXCH,		OPCHndlrFPU_FXCH,		OPCHndlrFPU_FXCH,		OPCHndlrFPU_FXCH,		OPCHndlrFPU_FXCH,		OPCHndlrFPU_FXCH,
 /*D9,D0*/	OPCHndlrFPU_FNOP,		OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*D9,D8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*D9,E0*/	OPCHndlrFPU_FCHS,		OPCHndlrFPU_FABS,		OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_FTST,		OPCHndlrFPU_FXAM,		OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*D9,E8*/	OPCHndlrFPU_Const,		OPCHndlrFPU_Const,		OPCHndlrFPU_Const,		OPCHndlrFPU_Const,		OPCHndlrFPU_Const,		OPCHndlrFPU_Const,		OPCHndlrFPU_Const,		OPCHndlrFPU_Invalid,
 /*D9,F0*/	OPCHndlrFPU_LgExSc,		OPCHndlrFPU_LgExSc,		OPCHndlrFPU_Trig,		OPCHndlrFPU_Trig,		OPCHndlrFPU_FXTRACT,	OPCHndlrFPU_FPREM,		OPCHndlrFPU_Ctl,		OPCHndlrFPU_Ctl,
 /*D9,F8*/	OPCHndlrFPU_FPREM,		OPCHndlrFPU_LgExSc,		OPCHndlrFPU_FSQRT,		OPCHndlrFPU_Trig,		OPCHndlrFPU_FRNDINT,	OPCHndlrFPU_LgExSc,		OPCHndlrFPU_Trig,		OPCHndlrFPU_Trig,

 /*DA,C0*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,
 /*DA,C8*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,
 /*DA,D0*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,
 /*DA,D8*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,
 /*DA,E0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DA,E8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DA,F0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DA,F8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,

 /*DB,C0*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,	
 /*DB,C8*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,
 /*DB,D0*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,
 /*DB,D8*/	OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,		OPCHndlrFPU_FCMOV,
 /*DB,E0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Ctl,		OPCHndlrFPU_Ctl,		OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DB,E8*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*DB,F0*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*DB,F8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,

 /*DC,C0*/	OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,
 /*DC,C8*/	OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,
 /*DC,D0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DC,D8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DC,E0*/	OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,
 /*DC,E8*/	OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,
 /*DC,F0*/	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,
 /*DC,F8*/	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,

 /*DD,C0*/	OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,		OPCHndlrFPU_CtlOp,
 /*DD,C8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DD,D0*/	OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,
 /*DD,D8*/	OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,		OPCHndlrFPU_FStore,
 /*DD,E0*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*DD,E8*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*DD,F0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DD,F8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,

 /*DE,C0*/	OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,		OPCHndlrFPU_FADD,
 /*DE,C8*/	OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,		OPCHndlrFPU_FMUL,
 /*DE,D0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DE,D8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DE,E0*/	OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,
 /*DE,E8*/	OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,		OPCHndlrFPU_FSUB,
 /*DE,F0*/	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,
 /*DE,F8*/	OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,		OPCHndlrFPU_FDIV,

 /*DF,C0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DF,C8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DF,D0*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DF,D8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DF,E0*/	OPCHndlrFPU_CtlOp,		OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,
 /*DF,E8*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*DF,F0*/	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,	OPCHndlrFPU_FCmpReal,
 /*DF,F8*/	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,	OPCHndlrFPU_Invalid,

 };

// IASD Vol2 - Table 3-1
// Look for #defines for indexes into these arrays
WCHAR awszRegCodes8[8][3] = {L"al", L"cl", L"dl", L"bl", L"ah", L"ch", L"dh", L"bh"};
WCHAR awszRegCodes16[8][3] = {L"ax", L"cx", L"dx", L"bx", L"sp", L"bp", L"si", L"di"};
WCHAR awszRegCodes32[8][4] = {L"eax", L"ecx", L"edx", L"ebx", L"esp", L"ebp", L"esi", L"edi"};

// IASD Vol2 - Table B-6: segment registers
WCHAR awszSRegCodes[8][3] = {L"es", L"cs", L"ss", L"ds", L"fs", L"gs", L"!s", L"!s"};

// IASD Vol2 - Table B-7: control registers
WCHAR awszCRegCodes[8][4] = {L"cr0", L"!c", L"cr2", L"cr3", L"cr4", L"!c", L"!c", L"!c"};

// IASD Vol2 - Table B-7: debug registers
WCHAR awszDRegCodes[8][4] = {L"dr0", L"dr1", L"dr2", L"dr3", L"!d", L"!d", L"dr6", L"dr7"};

// IASD Vol1 - 8.1.1 MMX Registers
WCHAR awszMMXRegCodes[8][4] = {L"mm0", L"mm1", L"mm2", L"mm3", L"mm4", L"mm5", L"mm6", L"mm7" };

WCHAR awszXMMRegCodes[8][5] = {L"xmm0", L"xmm1", L"xmm2", L"xmm3", L"xmm4", L"xmm5", L"xmm6", L"xmm7" };

// operand pointer strings
static WCHAR awszPtrStr[][MAX_PTR_STR] = {L"", L"byte ptr ", L"word ptr ", L"dword ptr ", 
											L"fword ptr ", L"qword ptr ", L"tbyte ptr ",
											L"mmword ptr ", L"xmmword ptr "};


/* fDisassembler()
 * Entry point for the DasmEngine. Calls fDoDisassembly() for all the
 * code sections specified in the argument pCodeLocs.
 *
 * Args:
 *		pCodeLocs: Pointer to the struct containing the array of address
 *					and size of all parts of code in the .text section.
 *		dwVirtCodeBase: DWORD value indicating the size of the
 *			code section. Required to stop the disassembler at the
 *			end of the code section(.text).
 *
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 *
 * Notes: Under construction
 */
BOOL fDisassembler(NCODE_LOCS *pCodeLocs, DWORD dwVirtCodeBase)
{
	ASSERT(pCodeLocs != NULL);
	
	wprintf_s(L"\n\n**** Disassembly starting from FilePtr = 0x%08x ****\n", 
				(long)pCodeLocs->aNonCodeLocs[0].dwFilePointer - (long)pCodeLocs->hFileBase);
	
	for(INT i = 0; i < pCodeLocs->nNonCodeParts; ++i)
		if(pCodeLocs->aNonCodeLocs[i].iPartType == CODE_PART_CODE)
			fDoDisassembly((DWORD*)pCodeLocs->aNonCodeLocs[i].dwFilePointer,
				pCodeLocs->aNonCodeLocs[i].dwPartSize, dwVirtCodeBase);

	return FALSE;
	
}// fDisassembler()

/* fDoDisassembly()
 * The DASM module calls this fDoDisassembly() with a pointer to 
 * the code section(.text) of the executable file. This is where 
 * the entry point must be.
 * fDoDisassembly() is modeled as a state machine.
 *
 * Args:
 *		pdwCodeSection: DWORD* pointer to the code section in the 
 *			memory-mapped PE file being disassembled.
 *		dwSizeOfCodeSection: DWORD value indicating the size of the
 *			code section. Required to stop the disassembler at the
 *			end of the code section(.text).
 *					
 *				
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fDoDisassembly(DWORD *pdwCodeSection, DWORD dwSizeOfCodeSection,
					DWORD dwVirtCodeBase)
{
	ASSERT(pdwCodeSection != NULL);

    DASMSTATE stDasmState;

	PBYTE pbEndOfCode = NULL;

    // Initialize DASMSTATE
    ZeroMemory(&stDasmState, sizeof(stDasmState));
    stDasmState.fRunning = TRUE;
    stDasmState.dsNextState = DASM_STATE_RESET;

    stDasmState.pbCurrentCodePtr = (BYTE*)pdwCodeSection;
    pbEndOfCode = stDasmState.pbCurrentCodePtr + dwSizeOfCodeSection;
    stDasmState.lDelta = (long)dwVirtCodeBase - (long)pdwCodeSection;

	wprintf_s(L"\n* Disassembly of code section *\n");

	// State machine is running until we encounter the end of code
	// section or an error occurs.
	while(1)
	{
		switch(stDasmState.dsNextState)
		{
			case DASM_STATE_RESET:
			{
				// Begin disassembly of a new instruction.
				// Clear out pstState->insCurIns.
				memset(&stDasmState.insCurIns, 0, sizeof(stDasmState.insCurIns));
				stDasmState.nBytesCurIns = 0;

				// Check if we have reached the end of code section
				// before beginning decoding of the next byte
				if(stDasmState.pbCurrentCodePtr >= pbEndOfCode)
					goto AFT_WHILE;
				else
					stDasmState.dsNextState = DASM_STATE_PREFIX;
				break;
			}

			case DASM_STATE_PREFIX:
			{
				if(!fStatePrefix(&stDasmState))
					stDasmState.dsNextState = DASM_STATE_ERROR;

				// If fStatePrefix did not return FALSE, then the next state
				// was already determined in that function.

				break;
			}

			case DASM_STATE_OPCODE:
			{
				if(!fStateOpcode(&stDasmState))
				{
					//dsNextState = DASM_STATE_ERROR;
					#ifdef _DEBUG
						wprintf_s(L"fDoDisassembly(): OPCODE error. Continuing...\n");
					#endif

					// simply print the offending opcode byte and move on
					fStateDumpOnOpcodeError(&stDasmState);

					// Let pstState->pbCurrentCodePtr point to the byte after the current 
					// opcode/prefix byte processed
					stDasmState.pbCurrentCodePtr = stDasmState.pbCurrentCodePtr - stDasmState.nBytesCurIns + 1;
					stDasmState.dsNextState = DASM_STATE_RESET;
				}
				break;
			}

			case DASM_STATE_MOD_RM:
			{
				if(!fStateModRM(&stDasmState))
					stDasmState.dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_SIB:
			{
				if(!fStateSIB(&stDasmState))
					stDasmState.dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_DISP:
			{
				if(!fStateDisp(&stDasmState))
					stDasmState.dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_IMM:
			{
				if(!fStateImm(&stDasmState))
					stDasmState.dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_DUMP:
			{
				if(stDasmState.insCurIns.fpvCallback)
					stDasmState.insCurIns.fpvCallback(&stDasmState);

				if(!fStateDump(&stDasmState))
					stDasmState.dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_ERROR:
			{
				wprintf_s(L"fDoDisassembly(): ERROR state. Aborting...\n");
				goto AFT_WHILE;
			}

			default:
			{
				// We will never reach here, typically.
				wprintf_s(L"fDoDisassembly(): Invalid state: %d\n", stDasmState.dsNextState);
				goto AFT_WHILE;
			}

		}// switch(bCurState)

	}// while(1)

AFT_WHILE:
	wprintf_s(L"\n**** End of disassembly ****\n");
	wprintf_s(L"Code begin        : %08Xh\n", (DWORD)pdwCodeSection);
	wprintf_s(L"Code size         : %Xh\n", dwSizeOfCodeSection);
	wprintf_s(L"Last byte decoded : %08Xh\n", (DWORD)stDasmState.pbCurrentCodePtr-1);
	wprintf_s(L"Bytes decoded     : %Xh\n", (DWORD)stDasmState.pbCurrentCodePtr - (DWORD)pdwCodeSection);

	return TRUE;
}// fDoDisassembly

/* fDasmDisassemble()
 * 
 * fDasmDisassemble() is modeled as a state machine.
 *
 * Args:
 *		pbCodeBegin: 
 *		dwSizeOfCodeSection: DWORD value indicating the size of the
 *			code section. Required to stop the disassembler at the
 *			end of the code section(.text).
 *      nInstToDisassemble:
 *      fStopAtFunctionEnd:
 *      dwTargetAddressBegin:
 *					
 *				
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fDasmDisassemble(
    PBYTE pbCodeBegin, 
    DWORD dwSizeOfCodeSection,
    INT nInstToDisassemble, 
    BOOL fStopAtFunctionEnd,
    DWORD dwTargetAddressBegin,
    PINT piReturnStatus)
{
	return FALSE;
}

/* fDasmDisassembleOne()
 * 
 * Disassemble one instruction at a time and return the statement as a string.
 *
 * Args:
 *		pbCodeBegin: 
 *		dwSizeOfCodeSection: DWORD value indicating the size of the
 *			code section. Required to stop the disassembler at the
 *			end of the code section(.text).
 *      nInstToDisassemble:
 *      fStopAtFunctionEnd:
 *      dwTargetAddressBegin:
 *					
 *				
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fDasmDisassembleOne(
    __inout PDASMSTATE pstDasmState,
    __in PBYTE pbCodeBegin, 
    DWORD dwSizeOfCodeSection,
    BOOL fStopAtFunctionEnd,
    DWORD dwTargetAddressBegin,
    __out PINT piReturnStatus)
{
    ASSERT(pbCodeBegin);
    ASSERT(pstDasmState);

    BOOL fRetVal = TRUE;

    DBG_UNREFERENCED_PARAMETER(piReturnStatus);
    DBG_UNREFERENCED_PARAMETER(fStopAtFunctionEnd);

    if(pstDasmState->structInitialized != MAGIC_DASMSTATE_STRUCT)
    {
        // Initialize DASMSTATE for the first call
        ZeroMemory(pstDasmState, sizeof(DASMSTATE));
        pstDasmState->structInitialized = MAGIC_DASMSTATE_STRUCT;

        pstDasmState->pbCurrentCodePtr = pbCodeBegin;
        pstDasmState->pbEndOfCode = pbCodeBegin + dwSizeOfCodeSection;

        pstDasmState->lDelta = (long)dwTargetAddressBegin - (long)pbCodeBegin;

        ASSERT(pbCodeBegin < (pbCodeBegin + dwSizeOfCodeSection));
    }

    pstDasmState->fRunning = TRUE;
    pstDasmState->dsNextState = DASM_STATE_RESET;

	// State machine is running until we have disassembled one instruction
    while(1)
    {
		switch(pstDasmState->dsNextState)
		{
			case DASM_STATE_RESET:
			{
				// Clear out pstState->insCurIns
				memset(&pstDasmState->insCurIns, 0, sizeof(pstDasmState->insCurIns));
				pstDasmState->nBytesCurIns = 0;

				// Check if we have reached the end of code section
				// before beginning decoding of the next byte
				if(pstDasmState->pbCurrentCodePtr >= pstDasmState->pbEndOfCode)
                {
                    // This is an error because we do not expect to hit end of code
                    // when disassembling one instruction at a time.
                    // TODO: Send back the return status saying we need more code bytes
                    fRetVal = FALSE;
					goto AFT_WHILE;
                }
				else
					pstDasmState->dsNextState = DASM_STATE_PREFIX;
				break;
			}

			case DASM_STATE_PREFIX:
			{
				if(!fStatePrefix(pstDasmState))
					pstDasmState->dsNextState = DASM_STATE_ERROR;

				// If fStatePrefix did not return FALSE, then the next state
				// was already determined in that function.

				break;
			}

			case DASM_STATE_OPCODE:
			{
				if(!fStateOpcode(pstDasmState))
				{
                    // This function is disassembling one instruction at a time, so we want to 
                    // indicate error if we cannot decode an opcode

					pstDasmState->dsNextState = DASM_STATE_ERROR;
			        dbgwprintf(L"fDoDisassembly(): OPCODE error. Continuing...\n");

					// simply print the offending opcode byte and move on
					//fStateDumpOnOpcodeError(pstDasmState);

					// Let pstState->pbCurrentCodePtr point to the byte after the current 
					// opcode/prefix byte processed so that on the next call we will decode
                    // starting from here
					pstDasmState->pbCurrentCodePtr = pstDasmState->pbCurrentCodePtr - pstDasmState->nBytesCurIns + 1;
					//pstDasmState->dsNextState = DASM_STATE_RESET;
				}
				break;
			}

			case DASM_STATE_MOD_RM:
			{
				if(!fStateModRM(pstDasmState))
					pstDasmState->dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_SIB:
			{
				if(!fStateSIB(pstDasmState))
					pstDasmState->dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_DISP:
			{
				if(!fStateDisp(pstDasmState))
					pstDasmState->dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_IMM:
			{
				if(!fStateImm(pstDasmState))
					pstDasmState->dsNextState = DASM_STATE_ERROR;
				break;
			}

			case DASM_STATE_DUMP:
			{
				if(pstDasmState->insCurIns.fpvCallback)
					pstDasmState->insCurIns.fpvCallback(pstDasmState);

				if(!fStateDump_ToString(pstDasmState))
					pstDasmState->dsNextState = DASM_STATE_ERROR;
				
                // Disassembling one instruction at a time, so exit state machine
                goto AFT_WHILE;

			}

			case DASM_STATE_ERROR:
			{
				dbgwprintf(L"fDoDisassembly(): ERROR state. Aborting...\n");
                fRetVal = FALSE;
				goto AFT_WHILE;
			}

			default:
			{
				// We will never reach here, typically.
				dbgwprintf(L"fDoDisassembly(): Invalid state: %d\n", pstDasmState->dsNextState);
                fRetVal = FALSE;
				goto AFT_WHILE;
			}

		}// switch(bCurState)
    }

AFT_WHILE:
    
    return fRetVal;
}

/* fStateReset()
 * This is the state that the state machine begins to operate in.
 * This is the state that is reached after disassembling an instruction
 * and before starting the disassembly of the next instruction, unless
 * an error occured.
 *
 * Args:
 *			
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fStateReset(PDASMSTATE pstState)
{
	return TRUE;
}// fStateReset()


/* fStatePrefix()
 * This state processes the prefix(es) in the instruction.
 *
 * Args:
 *			
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fStatePrefix(PDASMSTATE pstState)
{
	INT nPrefixes = 0;

	// There may be upto 4 prefixes of 1byte each.
	// We must scan forward until we find a byte that
	// is not a prefix.
	while( Util_fIsPrefix(*pstState->pbCurrentCodePtr) )
	{
		++nPrefixes;

		switch(*pstState->pbCurrentCodePtr)
		{
			case 0xf0:
			case 0xf2:
			case 0xf3:
				pstState->insCurIns.wPrefixTypes |= PREFIX_LOCKREP;
				pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] = *pstState->pbCurrentCodePtr;
				break;

			case 0x2e:
			case 0x36:
			case 0x3e:
			case 0x26:
			case 0x64:
			case 0x65:
				pstState->insCurIns.wPrefixTypes |= PREFIX_SEG;
				pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG] = *pstState->pbCurrentCodePtr;
				break;

			case 0x66:
				pstState->insCurIns.wPrefixTypes |= PREFIX_OPSIZE;
				pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] = *pstState->pbCurrentCodePtr;
				break;

			case 0x67:
				pstState->insCurIns.wPrefixTypes |= PREFIX_ADSIZE;
				pstState->insCurIns.abPrefixes[PREFIX_INDEX_ADSIZE] = *pstState->pbCurrentCodePtr;
				break;

			default:
				wprintf_s(L"fStatePrefix(): Unrecognized prefix type %u\n", *pstState->pbCurrentCodePtr);
				break;
		}// switch(*pstState->pbCurrentCodePtr)
		
		#ifdef _DEBUG
			wprintf_s(L"PREFIX: %xh\n", *pstState->pbCurrentCodePtr);
		#endif

		// We have read 1byte
		++(pstState->pbCurrentCodePtr);
		++pstState->nBytesCurIns;

	}

	pstState->insCurIns.nPrefix = nPrefixes;
	if(nPrefixes > 0)
		pstState->insCurIns.fPrefix = TRUE;

	// No more prefixes to process.
	// pstState->pbCurrentCodePtr is now pointing to an opcode.
	pstState->dsNextState = DASM_STATE_OPCODE;

	return TRUE;

}// fStatePrefix()


/* fStateOpcode()
 * This state processes the opcode(s) in the instruction.
 *
 * Args:
 *			
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fStateOpcode(PDASMSTATE pstState)
{
	// Lookup instruction string using opcode
	WORD wIndex;

	// Differentiate whether it is a 1byte/2byte opcode
	if(*pstState->pbCurrentCodePtr == OPC_2BYTE_ESCAPE)
	{
		return OPCHndlr_2ByteHandler(pstState, *pstState->pbCurrentCodePtr);
	}
	
	// 1byte opcode
	pstState->bFullOpcode = *pstState->pbCurrentCodePtr;

	// Separate out the first and second 4bits and use them
	// as indexes into the afpOpcHndlrs function pointer matrix
	pstState->bOpcodeLow = *pstState->pbCurrentCodePtr & 0x0f;
	pstState->bOpcodeHigh = (*pstState->pbCurrentCodePtr & 0xf0) >> 4;

	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;

	// store the d and w bits in INS_SPLIT
	Util_vGetDWBits(pstState->bFullOpcode, &pstState->insCurIns.bDBit, &pstState->insCurIns.bWBit);

	// Construct the output string starting with the instruction mnemonic
	wIndex = (pstState->bOpcodeHigh * 16) + pstState->bOpcodeLow;
	StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszOpcodeLUT1[wIndex]);
	
	// function call
	return afpOpcHndlrs[pstState->bOpcodeHigh][pstState->bOpcodeLow](pstState, pstState->bFullOpcode);

}// fStateOpcode()


/* OPCHndlr_2ByteHandler()
 * This state processes the 2byte opcodes.
 *
 * Args:
 *
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL OPCHndlr_2ByteHandler(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		if(bOpcode != 0x0f)
		{
			wprintf_s(L"OPCHndlr_2ByteHandler(): Not a 2byte escape code: %xh\n", bOpcode);
			return FALSE;
		}
	#endif

	WORD wIndex;

	// Go past 0x0f
	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;

	// pstState->pbCurrentCodePtr is now pointing to the second byte of the opcode;
	// i.e., the byte after 0x0f
	pstState->bFullOpcode = *pstState->pbCurrentCodePtr;
	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;

	// pstState->pbCurrentCodePtr now points to the byte after the second opcode byte
	
	// Separate out the first and second 4bits and use them
	// as indexes into the afpOpcHndlrs2 function pointer matrix
	pstState->bOpcodeLow = pstState->bFullOpcode & 0x0f;
	pstState->bOpcodeHigh = (pstState->bFullOpcode & 0xf0) >> 4;

	// store the d and w bits in INS_SPLIT
	Util_vGetDWBits(pstState->bFullOpcode, &pstState->insCurIns.bDBit, &pstState->insCurIns.bWBit);
	
	// Construct the output string starting with the instruction mnemonic
	wIndex = (pstState->bOpcodeHigh * 16) + pstState->bOpcodeLow;
	StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszOpcodeLUT2[wIndex]);

	// function call
	return afpOpcHndlrs2[pstState->bOpcodeHigh][pstState->bOpcodeLow](pstState, pstState->bFullOpcode);

}// OPCHndlr_2ByteHandler()


/* OPCHndlr_3ByteHandler()
 * This state processes the 2byte opcodes.
 *
 * Args:
 *
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL OPCHndlr_3ByteHandler(PDASMSTATE pstState, BYTE bOpcode)
{
	WORD wIndex;

	// pstState->pbCurrentCodePtr is now pointing to the third byte of the opcode;
	// i.e., the byte after 0x3a or 0x38
	pstState->bFullOpcode = *pstState->pbCurrentCodePtr;
	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;

	if(bOpcode == 0x38)
	{
		switch(pstState->bFullOpcode)
		{
			case 0xDB:
			case 0xDC:
			case 0xDD:
			case 0xDE:
			case 0xDF: return OPCHndlrAES(pstState, pstState->bFullOpcode); break;
			default:
				wprintf_s(L"OPCHndlr_3ByteHandler(): Invalid third byte %xh %xh\n", bOpcode, pstState->bFullOpcode);
				return FALSE;
		}
	}
	else if(bOpcode == 0x3A)
	{
		switch(pstState->bFullOpcode)
		{
			case 0xDF: return OPCHndlrAES(pstState, pstState->bFullOpcode); break;
			default:
				wprintf_s(L"OPCHndlr_3ByteHandler(): Invalid third byte %xh %xh\n", bOpcode, pstState->bFullOpcode);
				return FALSE;
		}
	}
	else
	{
		wprintf_s(L"OPCHndlr_3ByteHandler(): Not a 3byte escape code: %xh\n", bOpcode);
		return FALSE;
	}

}// OPCHndlr32ByteHandler()


#ifdef UNIT_TESTS_ONLY

/*	**** FPU Unit Testing **** */
void DEngine_FPUUnitTest()
{	
	INT iOpcode;
	INT iModRM;

	wprintf_s(L"\n*************** RegEx ***************\n");

	for(iOpcode = 0xD8; iOpcode <= 0xDF; ++iOpcode)
	{
		pstState->bFullOpcode = (BYTE)iOpcode;
		pstState->bOpcodeLow = pstState->bFullOpcode & 0x0f;
		pstState->bOpcodeHigh = (pstState->bFullOpcode & 0xf0) >> 4;

		for(INT iReg = 0; iReg <= 7; ++iReg)
		{
			pstState->bFPUModRM = iReg;
			pstState->bFPUModRM = pstState->bFPUModRM << 3;
			OPCHndlrFPU_All(pstState->bFullOpcode);
		}

		wprintf_s(L"\n");
	}// for bOpcode

	wprintf_s(L"\n*************** FullEx ***************\n");
	for(iOpcode = 0xD8; iOpcode <= 0xDF; ++iOpcode)
	{
		pstState->bFullOpcode = (BYTE)iOpcode;
		pstState->bOpcodeLow = pstState->bFullOpcode & 0x0f;
		pstState->bOpcodeHigh = (pstState->bFullOpcode & 0xf0) >> 4;

		for(iModRM = 0xC0; iModRM <= 0xFF; ++iModRM)
		{
			pstState->bFPUModRM = iModRM;
			OPCHndlrFPU_All(pstState->bFullOpcode);
		}
		wprintf_s(L"\n");
	}// for bOpcode
		
	return;
}

/*	**** MMX Unit Testing **** */
void DEngine_MMXUnitTest()
{
	// second opcode byte values
	static BYTE abMMXOpcodes[] = {	0x0F, 0x60,
									0x0F, 0x61,
									0x0F, 0x62,
									0x0F, 0x63,
									0x0F, 0x64,
									0x0F, 0x65,
									0x0F, 0x66,
									0x0F, 0x67,
									0x0F, 0x67,
									0x0F, 0x68,
									0x0F, 0x69,
									0x0F, 0x6A,
									0x0F, 0x6B,
									0x0F, 0x6E,
									0x0F, 0x6F,

									0x0F, 0x70,
									0x0F, 0x71,
									0x0F, 0x72,
									0x0F, 0x73,
									0x0F, 0x74,
									0x0F, 0x75,
									0x0F, 0x76,
									0x0F, 0x77,
									0x0F, 0x7E,
									0x0F, 0x7F,

									0x0F, 0xC4,
									0x0F, 0xC5,

									0x0F, 0xD1,
									0x0F, 0xD2,
									0x0F, 0xD3,
									0x0F, 0xD5,
									0x0F, 0xD7,
									0x0F, 0xD8,
									0x0F, 0xD9,
									0x0F, 0xDA,
									0x0F, 0xDB,
									0x0F, 0xDC,
									0x0F, 0xDD,
									0x0F, 0xDE,
									0x0F, 0xDF,

									0x0F, 0xE0,
									0x0F, 0xE1,
									0x0F, 0xE2,
									0x0F, 0xE3,
									0x0F, 0xE4,
									0x0F, 0xE5,
									0x0F, 0xE8,
									0x0F, 0xE9,
									0x0F, 0xEA,
									0x0F, 0xEB,
									0x0F, 0xEC,
									0x0F, 0xED,
									0x0F, 0xEE,
									0x0F, 0xEF,

									0x0F, 0xF1,
									0x0F, 0xF2,
									0x0F, 0xF3,
									0x0F, 0xF5,
									0x0F, 0xF6,
									0x0F, 0xF8,
									0x0F, 0xF9,
									0x0F, 0xFA,
									0x0F, 0xFC,
									0x0F, 0xFD,
									0x0F, 0xFE };

	//BYTE *pCurOpcode = abMMXOpcodes;
	INT nOpcodes = _countof(abMMXOpcodes)/2;
	for(INT i = 0; i < nOpcodes; ++i)
	{
		pstState->pbCurrentCodePtr = abMMXOpcodes + i*2;
		fStateOpcode();
	}

	return;
}

/*	**** SSE Unit Testing **** */
void DEngine_SSEUnitTest()
{
	// second opcode byte values
	static BYTE abSSEOpcodes[] = {	0x0F, 0x10,
									0x0F, 0x11,
									0x0F, 0x12,
									0x0F, 0x13,
									0x0F, 0x14,
									0x0F, 0x15,
									0x0F, 0x16,
									0x0F, 0x17,

									0x0F, 0x28,
									0x0F, 0x29,
									0x0F, 0x2A,
									0x0F, 0x2B,
									0x0F, 0x2C,
									0x0F, 0x2D,
									0x0F, 0x2E,
									0x0F, 0x2F,

									0x0F, 0x50,
									0x0F, 0x51,
									0x0F, 0x52,
									0x0F, 0x53,
									0x0F, 0x54,
									0x0F, 0x55,
									0x0F, 0x56,
									0x0F, 0x57,
									0x0F, 0x58,
									0x0F, 0x59,
									0x0F, 0x5C,
									0x0F, 0x5D,
									0x0F, 0x5E,
									0x0F, 0x5F,

									0x0F, 0xAE,
									0x0F, 0xC2,
									0x0F, 0xC6,

									0x0F, 0xE7,
									0x0F, 0xF7 };

	//BYTE *pCurOpcode = abSSEOpcodes;
	INT nOpcodes = _countof(abSSEOpcodes)/2;
	for(INT i = 0; i < nOpcodes; ++i)
	{
		pstState->pbCurrentCodePtr = abSSEOpcodes + i*2;
		fStateOpcode();
	}

	return;
}
#endif

/*
 * OPCHndlrFPU_All()
 * This function is called for all opcodes between D8h and DFh,
 * which are the FPU instructions.
 * 
 */
BOOL OPCHndlrFPU_All(PDASMSTATE pstState, BYTE bOpcode)
{
	// All FPU instructions have 0xD as the high nibble
	ASSERT(pstState->bOpcodeHigh == 0xD);

	// Figure out how this instruction uses the ModRM byte:
	// - All bits as opcode extension OR
	// - Only 'reg' field as opcode extension

#ifndef UNIT_TESTS_ONLY
	pstState->bFPUModRM = *pstState->pbCurrentCodePtr;
#endif

	// pstState->pbCurrentCodePtr now points to the next instruction.
	// pstState->pbCurrentCodePtr need not be modified in any of the
	// individual FPU instruction functions.

	if(pstState->bFPUModRM <= 0xBF)
		return OPCHndlrFPU_ModRMRegEx(pstState, bOpcode);

	// ModRM lies between C0h and FFh
	return OPCHndlrFPU_ModRMFullEx(pstState, bOpcode);
}

BOOL OPCHndlrFPU_ModRMRegEx(PDASMSTATE pstState, BYTE bOpcode)
{
	ASSERT(pstState->bFPUModRM >= 0x00 && pstState->bFPUModRM <= 0xBF);	// always??

	// Value of ModRM is stored into global variable pstState->bFPUModRM
	// pstState->pbCurrentCodePtr is now pointing to the ModRM byte

	INT iRowIndex;
	INT iInsStrIndex;

	Util_vSplitModRMByte(pstState->bFPUModRM, NULL, &pstState->bFPUReg, NULL);
	ASSERT(pstState->bFPUReg >= 0 && pstState->bFPUReg <= 7);

	/*
	 * Subtract the opcode by 0xD8 to obtain a zero-based
	 * opcode to use as row index into afpFPUModRMRegEx.
	 * Use the bReg value as the column index.
	 */
	iRowIndex = bOpcode - 0xD8;

	iInsStrIndex = iRowIndex * 8 + pstState->bFPUReg;
	StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszFPUOpcodeLUTRegEx[iInsStrIndex]);

	return afpFPUModRMRegEx[iRowIndex][pstState->bFPUReg](pstState, bOpcode);
}

BOOL OPCHndlrFPU_ModRMFullEx(PDASMSTATE pstState, BYTE bOpcode)
{
	ASSERT(pstState->bFPUModRM >= 0xC0 && pstState->bFPUModRM <= 0xFF);	// always??

	// Value of ModRM is stored into global variable pstState->bFPUModRM

	/*
	 * pstState->pbCurrentCodePtr is now pointing to the ModRM byte.
	 * Move past the ModRM byte because it is not used
	 * to calculate the effective address but is just an
	 * opcode extension. We are going to DUMP state next.
	 */
	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;
	
	/*
	 * Subtract the opcode by 0xD8 to obtain a zero-based
	 * opcode to use as row index into afpFPUModRMRegEx.
	 * Subtract the ModRM value by 0xC0 to obtain a zero-based
	 * ModRM to use as column index.
	 */
	BYTE bRowIndex = bOpcode - 0xD8;
	BYTE bColIndex = pstState->bFPUModRM - 0xC0;
	INT iInsStrIndex = bRowIndex * 0x40 + bColIndex;

	StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszFPUOpcodeLUTFullEx[iInsStrIndex]);

	return afpFPUModRMFullEx[bRowIndex][bColIndex](pstState, bOpcode);
}


/*
 * *************************
 *		Begin MODRM state	
 * *************************
 */

/* fStateModRM()
 * This state processes the ModRM byte in the instruction.
 *
 * Args:
 *			
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fStateModRM(PDASMSTATE pstState)
{
	BYTE bModRM;
	BYTE bMod;
	BYTE bReg;
	BYTE bRM;
	BOOL fRetVal = TRUE;

	bModRM = *pstState->pbCurrentCodePtr;
	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;

	#ifdef _DEBUG
		wprintf_s(L"fStateModRM(): %xh\n", bModRM);
	#endif

	// pstState->pbCurrentCodePtr now points to either a disp/SIB/IMM

	pstState->insCurIns.bModRM = bModRM;

	// split the ModRM byte into individual fields
	Util_vSplitModRMByte(bModRM, &bMod, &bReg, &bRM);

	// Check the ModRM type
	if(pstState->insCurIns.bModRMType == MODRM_TYPE_DIGIT)
	{
		// Only Mod+RM bits are to be evaluated
		// Only 1 operand specified using ModRM byte
		fRetVal = MODRM_fTypeDigit(pstState, bModRM, bMod, bRM);
	}

	else if(pstState->insCurIns.bModRMType == MODRM_TYPE_R)
	{
		// Two operands specified by Mod+RM and Reg bits
		fRetVal = MODRM_fTypeR(pstState, bModRM, bMod, bReg, bRM);
	}
	else
	{
		#ifdef _DEBUG
			wprintf_s(L"fStateModRM(): Invalid bModRMType %d\n", pstState->insCurIns.bModRMType);
			fRetVal = FALSE;
		#endif
	}

	// pstState->dsNextState has been determined in fSplIns()/TypeR()/TypeDigit()

	return fRetVal;

}// fStateModRM


/*
 * MODRM_fTypeDigit()
 *	Reg field is the opcode extension. Mod+RM fields give the effective 
 *	address of the operand. Next state will be set if this returns TRUE.
 */
BOOL MODRM_fTypeDigit(PDASMSTATE pstState, BYTE bModRM, BYTE bMod, BYTE bRM)
{
	BYTE bOprSize;
	OPRTYPE_RETVAL oprType;

	/*
	 * Only one operand from ModRM byte and that is stored as the src str
	 */

	if(pstState->insCurIns.fSpecialInstruction)// && 
	   //pstState->insCurIns.bSplInsType == SPL_INS_TYPE_SIZEHINT)
	{
		bOprSize = pstState->insCurIns.bOperandSizeSrc;
	}
	else
		bOprSize = MODRM_bGetOperandSize(pstState);

	// Set bImmType if required
	if(pstState->insCurIns.fImm && pstState->insCurIns.bImmType == IMM_UNDEF)
			pstState->insCurIns.bImmType = bOprSize;

	// If we have a special instruction with specific regs
	// No, specific regs is specified using /r ModRM type; never /digit
	// 081812: MMX regs are specified using 'reg' field also.
	oprType = MODRM_GetOperandFromModRM(bMod, bRM, bOprSize, pstState->insCurIns.bRegTypeSrc,
							pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc));

	if(oprType == OPRTYPE_ERROR)
	{
		#ifdef _DEBUG
			wprintf_s(L"MODRM_fTypeDigit(): Error retrieving operand: Mod = %d : RM = %d\n",
						bMod, bRM);
		#endif
		return FALSE;
	}

	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.OprTypeSrc = oprType;

	switch(oprType)
	{
		case OPRTYPE_MEM:
		{
			// Set ptr str
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSize];

			// Before going to DUMP, check whether there is a imm value
			if(pstState->insCurIns.fImm)
				pstState->dsNextState = DASM_STATE_IMM;
			else
				pstState->dsNextState = DASM_STATE_DUMP;
			break;
		}
		
		case OPRTYPE_MEM8:	// We have a displacement and must go to DISP state next
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSize];
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_8BIT;
			pstState->dsNextState = DASM_STATE_DISP;
			break;
		}

		case OPRTYPE_MEM32:	// disp32 follows
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSize];
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_32BIT;
			pstState->dsNextState = DASM_STATE_DISP;
			break;
		}

		case OPRTYPE_SIB:	// No displacement
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSize];
			pstState->dsNextState = DASM_STATE_SIB;
			break;
		}

		case OPRTYPE_SIB8:	// SIB follows, followed by disp8
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSize];
			pstState->insCurIns.fSIB = TRUE;
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_8BIT;
			pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
			break;
		}

		case OPRTYPE_SIB32:	// SIB follows, followed by disp32
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSize];
			pstState->insCurIns.fSIB = TRUE;
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_32BIT;
			pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
			break;
		}

		// todo: check whether there is a SIB byte and then the disp32
		case OPRTYPE_DISP32:	// disp32 follows
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSize];
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_32BIT;
			pstState->dsNextState = DASM_STATE_DISP;
			break;
		}

		case OPRTYPE_REG:
		{
			// Before going to DUMP, check whether there is a imm value
			if(pstState->insCurIns.fImm)
				pstState->dsNextState = DASM_STATE_IMM;
			else
				pstState->dsNextState = DASM_STATE_DUMP;
			break;
		}

		default:
			wprintf_s(L"MODRM_fTypeDigit(): Invalid oprType %d\n", oprType);
			return FALSE;
	
	}// switch(oprType)

	return TRUE;
}// MODRM_fTypeDigit()


/*
 * MODRM_fTypeR()
 *		** D-bit implications **
 *	If dbit = 1, reg	<-- Mod+RM	: src = Mod+RM
 *	If dbit = 0, Mod+RM	<-- reg		: src = Reg
 *
 *	Next state will be set if this returns TRUE.
 */
BOOL MODRM_fTypeR(PDASMSTATE pstState, BYTE bModRM, BYTE bMod, BYTE bReg, BYTE bRM)
{
	ASSERT(bMod >= 0 && bMod <= 3);
	ASSERT(bRM >= 0 && bRM <= 7);
	ASSERT(bReg >= 0 && bReg <= 7);

	// 2 different variables in order to handle fSplInstruction
	BYTE bOprSizeSrc;
	BYTE bOprSizeDes;

	// Eliminating redundant switch()
	BYTE bLocalOprSize;
	OPRTYPE_RETVAL LocalOprType;


	if(pstState->insCurIns.fSpecialInstruction)
	{
		bOprSizeSrc = pstState->insCurIns.bOperandSizeSrc;
		bOprSizeDes = pstState->insCurIns.bOperandSizeDes;
		#ifdef _DEBUG
			if(pstState->insCurIns.fImm && pstState->insCurIns.bImmType == IMM_UNDEF)
			{
				wprintf_s(L"MODRM_fTypeR(): fSpl bImmType UNDEF\n");
				return FALSE;
			}
		#endif
	}
	else
	{
		bOprSizeSrc = bOprSizeDes = MODRM_bGetOperandSize(pstState);
		if(pstState->insCurIns.fImm && pstState->insCurIns.bImmType == IMM_UNDEF)
			pstState->insCurIns.bImmType = bOprSizeSrc;
	}

	// There are two operands specified using the ModRM byte.
	// Get source and destination operands
	if(pstState->insCurIns.bDBit == 0)
	{
		// src = Reg; des = Mod+RM
		
		// If we have a special instruction with specific regs
		//if(pstState->insCurIns.fSpecialInstruction && pstState->insCurIns.bSplInsType == SPL_INS_TYPE_REG)
		//{
		//	pstState->insCurIns.OprTypeSrc = MODRM_GetOperandFromReg(bReg, bOprSizeSrc, pstState->insCurIns.bRegTypeSrc, 
		//								pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc));
		//	pstState->insCurIns.OprTypeDes = MODRM_GetOperandFromModRM(bMod, bRM, bOprSizeDes, pstState->insCurIns.bRegTypeDest, 
		//								pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes));
		//}
		//else
		//{
		//	pstState->insCurIns.OprTypeSrc = MODRM_GetOperandFromReg(bReg, bOprSizeSrc, REGTYPE_GREG, 
		//								pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc));
		//	pstState->insCurIns.OprTypeDes = MODRM_GetOperandFromModRM(bMod, bRM, bOprSizeDes, pstState->insCurIns.bRegTypeDest, 
		//								pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes));
		//}

		pstState->insCurIns.OprTypeSrc = MODRM_GetOperandFromReg(bReg, bOprSizeSrc, pstState->insCurIns.bRegTypeSrc, 
									pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc));
		pstState->insCurIns.OprTypeDes = MODRM_GetOperandFromModRM(bMod, bRM, bOprSizeDes, pstState->insCurIns.bRegTypeDest, 
									pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes));

		LocalOprType = pstState->insCurIns.OprTypeDes;
		bLocalOprSize = bOprSizeDes;

		//pstState->insCurIns.fSrcStrSet = TRUE;
		//pstState->insCurIns.fDesStrSet = TRUE;
		//switch(pstState->insCurIns.OprTypeDes)
		//{
		//	case OPRTYPE_MEM:
		//	{
		//		// Set ptr str
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeDes];
		//		// Before going to DUMP, check whether there is a imm value
		//		if(pstState->insCurIns.fImm)
		//		{
		//			pstState->dsNextState = DASM_STATE_IMM;
		//		}
		//		else
		//			pstState->dsNextState = DASM_STATE_DUMP;
		//		break;
		//	}
		//
		//	case OPRTYPE_MEM8:	// We have a displacement and must go to DISP state next
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeDes];
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_8BIT;
		//		pstState->dsNextState = DASM_STATE_DISP;
		//		break;
		//	}

		//	case OPRTYPE_MEM32:	// disp32 follows
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeDes];
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_32BIT;
		//		pstState->dsNextState = DASM_STATE_DISP;
		//		break;
		//	}

		//	case OPRTYPE_SIB:	// No displacement
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeDes];
		//		pstState->dsNextState = DASM_STATE_SIB;
		//		break;
		//	}

		//	case OPRTYPE_SIB8:	// SIB follows, followed by disp8
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeDes];
		//		pstState->insCurIns.fSIB = TRUE;
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_8BIT;
		//		pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
		//		break;
		//	}

		//	case OPRTYPE_SIB32:	// SIB follows, followed by disp32
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeDes];
		//		pstState->insCurIns.fSIB = TRUE;
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_32BIT;
		//		pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
		//		break;
		//	}

		//	case OPRTYPE_DISP32:	// disp32 follows
		//	{	
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeDes];
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_32BIT;
		//		pstState->dsNextState = DASM_STATE_DISP;
		//		break;
		//	}

		//	case OPRTYPE_REG:
		//	{
		//		// Before going to DUMP, check whether there is a imm value
		//		if(pstState->insCurIns.fImm)
		//			pstState->dsNextState = DASM_STATE_IMM;
		//		else
		//			pstState->dsNextState = DASM_STATE_DUMP;
		//		break;
		//	}

		//	default:
		//		wprintf_s(L"MODRM_fTypeR(): Invalid oprType %d\n", pstState->insCurIns.OprTypeDes);
		//		return FALSE;
	
		//}// switch(oprType)
	}
	else
	{
		// src = Mod+RM; des = Reg
		
		// If we have a special instruction with specific regs
		//if(pstState->insCurIns.fSpecialInstruction && pstState->insCurIns.bSplInsType == SPL_INS_TYPE_REG)
		//{
		//	pstState->insCurIns.OprTypeSrc = MODRM_GetOperandFromModRM(bMod, bRM, bOprSizeSrc, pstState->insCurIns.bRegTypeSrc,
		//								pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc));
		//	pstState->insCurIns.OprTypeDes = MODRM_GetOperandFromReg(bReg, bOprSizeDes, pstState->insCurIns.bRegTypeDest, 
		//								pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes));
		//}
		//else
		//{
		//	pstState->insCurIns.OprTypeSrc = MODRM_GetOperandFromModRM(bMod, bRM, bOprSizeSrc, pstState->insCurIns.bRegTypeSrc,
		//								pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc));
		//	pstState->insCurIns.OprTypeDes = MODRM_GetOperandFromReg(bReg, bOprSizeDes, pstState->insCurIns.bRegTypeDest, 
		//								pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes));
		//}

		pstState->insCurIns.OprTypeSrc = MODRM_GetOperandFromModRM(bMod, bRM, bOprSizeSrc, pstState->insCurIns.bRegTypeSrc,
									pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc));
		pstState->insCurIns.OprTypeDes = MODRM_GetOperandFromReg(bReg, bOprSizeDes, pstState->insCurIns.bRegTypeDest, 
									pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes));

		LocalOprType = pstState->insCurIns.OprTypeSrc;
		bLocalOprSize = bOprSizeSrc;

		//pstState->insCurIns.fSrcStrSet = TRUE;
		//pstState->insCurIns.fDesStrSet = TRUE;

		//switch(pstState->insCurIns.OprTypeSrc)
		//{
		//	case OPRTYPE_MEM:
		//	{
		//		// Set ptr str
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeSrc];
		//		// Before going to DUMP, check whether there is a imm value
		//		if(pstState->insCurIns.fImm)
		//			pstState->dsNextState = DASM_STATE_IMM;
		//		else
		//			pstState->dsNextState = DASM_STATE_DUMP;
		//		break;
		//	}
		//
		//	case OPRTYPE_MEM8:	// We have a displacement and must go to DISP state next
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeSrc];
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_8BIT;
		//		pstState->dsNextState = DASM_STATE_DISP;
		//		break;
		//	}

		//	case OPRTYPE_MEM32:	// disp32 follows
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeSrc];
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_32BIT;
		//		pstState->dsNextState = DASM_STATE_DISP;
		//		break;
		//	}

		//	case OPRTYPE_SIB:	// No displacement
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeSrc];
		//		pstState->dsNextState = DASM_STATE_SIB;
		//		break;
		//	}

		//	case OPRTYPE_SIB8:	// SIB follows, followed by disp8
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeSrc];
		//		pstState->insCurIns.fSIB = TRUE;
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_8BIT;
		//		pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
		//		break;
		//	}

		//	case OPRTYPE_SIB32:	// SIB follows, followed by disp32
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeSrc];
		//		pstState->insCurIns.fSIB = TRUE;
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_32BIT;
		//		pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
		//		break;
		//	}

		//	case OPRTYPE_DISP32:	// disp32 follows
		//	{
		//		pstState->insCurIns.pwszPtrStr = awszPtrStr[bOprSizeSrc];
		//		pstState->insCurIns.fDisp = TRUE;
		//		pstState->insCurIns.bDispType = DISP_32BIT;
		//		pstState->dsNextState = DASM_STATE_DISP;
		//		break;
		//	}

		//	case OPRTYPE_REG:
		//	{
		//		// Before going to DUMP, check whether there is a imm value
		//		if(pstState->insCurIns.fImm)
		//			pstState->dsNextState = DASM_STATE_IMM;
		//		else
		//			pstState->dsNextState = DASM_STATE_DUMP;
		//		break;
		//	}

		//	default:
		//		wprintf_s(L"MODRM_fTypeDigit(): Invalid oprType %d\n", pstState->insCurIns.OprTypeSrc);
		//		return FALSE;
	
		//}// switch(oprType)
	
	}// if(pstState->insCurIns.bDBit == 0)

	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.fDesStrSet = TRUE;

	switch(LocalOprType)
	{
		case OPRTYPE_MEM:
		{
			// Set ptr str
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bLocalOprSize];
			// Before going to DUMP, check whether there is a imm value
			if(pstState->insCurIns.fImm)
				pstState->dsNextState = DASM_STATE_IMM;
			else
				pstState->dsNextState = DASM_STATE_DUMP;
			break;
		}
		
		case OPRTYPE_MEM8:	// We have a displacement and must go to DISP state next
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bLocalOprSize];
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_8BIT;
			pstState->dsNextState = DASM_STATE_DISP;
			break;
		}

		case OPRTYPE_MEM32:	// disp32 follows
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bLocalOprSize];
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_32BIT;
			pstState->dsNextState = DASM_STATE_DISP;
			break;
		}

		case OPRTYPE_SIB:	// No displacement
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bLocalOprSize];
			pstState->dsNextState = DASM_STATE_SIB;
			break;
		}

		case OPRTYPE_SIB8:	// SIB follows, followed by disp8
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bLocalOprSize];
			pstState->insCurIns.fSIB = TRUE;
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_8BIT;
			pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
			break;
		}

		case OPRTYPE_SIB32:	// SIB follows, followed by disp32
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bLocalOprSize];
			pstState->insCurIns.fSIB = TRUE;
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_32BIT;
			pstState->dsNextState = DASM_STATE_SIB;	// SIB state must check for fDisp and transition
			break;
		}

		case OPRTYPE_DISP32:	// disp32 follows
		{
			pstState->insCurIns.pwszPtrStr = awszPtrStr[bLocalOprSize];
			pstState->insCurIns.fDisp = TRUE;
			pstState->insCurIns.bDispType = DISP_32BIT;
			pstState->dsNextState = DASM_STATE_DISP;
			break;
		}

		case OPRTYPE_REG:
		{
			// Before going to DUMP, check whether there is a imm value
			if(pstState->insCurIns.fImm)
				pstState->dsNextState = DASM_STATE_IMM;
			else
				pstState->dsNextState = DASM_STATE_DUMP;
			break;
		}

		default:
			wprintf_s(L"MODRM_fTypeR(): Invalid oprType %d\n", bLocalOprSize);
			return FALSE;
	
	}// switch(oprType)

	// pstState->dsNextState has been set
	
	return TRUE;
}


OPRTYPE_RETVAL MODRM_GetOperandFromModRM(BYTE bMod, BYTE bRM, BYTE bOprSize, BYTE bRegType, 
											__out WCHAR *pwszOprStr, __in DWORD dwOprStrCount)
{
	ASSERT(bMod >= 0 && bMod <= 3);
	ASSERT(bRM >= 0 && bRM <= 7);
	ASSERT(pwszOprStr != NULL);

	switch(bMod)
	{
		case 0:	// Effective address is in one of the GPRs/is a disp32/in a SIB byte
		{		// depending on the value of the RM field.
			if(bRM == EFFADDR_DISP32)
				return OPRTYPE_DISP32;
			
			if(bRM == EFFADDR_SIB)
				return OPRTYPE_SIB;
			
			// [eax] / [ecx] / ...
			StringCchPrintf(pwszOprStr, dwOprStrCount, L"%s", awszRegCodes32[bRM]);

			return OPRTYPE_MEM;
		}

		case 1:	// Effective address is in one of the GPRs with disp8/in a 
		{		// SIB byte with disp8 depending on the value of the RM field.			
			if(bRM == EFFADDR_SIB)
				return OPRTYPE_SIB8;
			
			// [eax] / [ecx] / ...
			StringCchPrintf(pwszOprStr, dwOprStrCount, L"%s", awszRegCodes32[bRM]);

			return OPRTYPE_MEM8;
		}

		case 2:	// Effective address is in one of the GPRs with disp32/in a 
		{		// SIB byte with disp32 depending on the value of the RM field.			
			if(bRM == EFFADDR_SIB)
				return OPRTYPE_SIB32;
			
			// [eax] / [ecx] / ...
			StringCchPrintf(pwszOprStr, dwOprStrCount, L"%s", awszRegCodes32[bRM]);

			return OPRTYPE_MEM32;
		}

		case 3:	// Operand is a GPR/CR/DR/...
		{		// handle CR/DR/SegReg: C,D,S regs are in 'reg' field; never in Mod+RM fields
				// 081812: but MMX regs may be specified using Mod+RM bits. So include a check
				// for this. (added one function argument:bRegType).
			if(bRegType == REGTYPE_GREG || bRegType == REGTYPE_UNDEF)
			{
				if(bOprSize == OPERANDSIZE_8BIT)
					StringCchCopy(pwszOprStr, dwOprStrCount, awszRegCodes8[bRM]);
				else if(bOprSize == OPERANDSIZE_16BIT)
					StringCchCopy(pwszOprStr, dwOprStrCount, awszRegCodes16[bRM]);
				else if(bOprSize == OPERANDSIZE_32BIT)
					StringCchCopy(pwszOprStr, dwOprStrCount, awszRegCodes32[bRM]);
			}
			else if(bRegType == REGTYPE_MMX)
			{
				StringCchCopy(pwszOprStr, dwOprStrCount, awszMMXRegCodes[bRM]);
			}
			else if(bRegType == REGTYPE_XMM)
			{
				StringCchCopy(pwszOprStr, dwOprStrCount, awszXMMRegCodes[bRM]);
			}
			else
				wprintf_s(L"MODRM_GetOperandFromModRM(): Invalid bRegType %d\n", bRegType);

			return OPRTYPE_REG;
		
		}// case 3

		default:
			wprintf_s(L"MODRM_GetOperandFromModRM(): Invalid bMod value %d\n", bMod);
			return OPRTYPE_ERROR;
	}// switch(bMod)

	return OPRTYPE_ERROR;

}// MODRM_GetOperandFromModRM()


OPRTYPE_RETVAL MODRM_GetOperandFromReg(BYTE bReg, BYTE bOprSize, BYTE bRegType, 
										__out WCHAR *pwszOprStr, __in DWORD dwOprStrCount)
{
	ASSERT(bReg >= 0 && bReg <= 7);
	ASSERT(pwszOprStr != NULL);

	// Operand is one of the GPR/CR/DR/SegReg...
	// handle CR/DR/SegReg
	// Yes, C,D,S regs are encoded in the 'reg' field
	// 081812: added check for MMX register
	switch(bRegType)
	{
		case REGTYPE_UNDEF:
		case REGTYPE_GREG:
		{
			if(bOprSize == OPERANDSIZE_8BIT)
				StringCchCopy(pwszOprStr, dwOprStrCount, awszRegCodes8[bReg]);
			else if(bOprSize == OPERANDSIZE_16BIT)
				StringCchCopy(pwszOprStr, dwOprStrCount, awszRegCodes16[bReg]);
			else if(bOprSize == OPERANDSIZE_32BIT)
				StringCchCopy(pwszOprStr, dwOprStrCount, awszRegCodes32[bReg]);
			else
			{
				wprintf_s(L"MODRM_GetOperandFromReg(): Invalid bOprSize %u\n", bOprSize);
				return OPRTYPE_ERROR;
			}
			break;
		}// UNDEF, GREG

		case REGTYPE_SREG:
			StringCchCopy(pwszOprStr, dwOprStrCount, awszSRegCodes[bReg]);
			break;

		case REGTYPE_CREG:
			StringCchCopy(pwszOprStr, dwOprStrCount, awszCRegCodes[bReg]);
			break;

		case REGTYPE_DREG:
			StringCchCopy(pwszOprStr, dwOprStrCount, awszDRegCodes[bReg]);
			break;
		
		case REGTYPE_MMX:
			StringCchCopy(pwszOprStr, dwOprStrCount, awszMMXRegCodes[bReg]);
			break;

		case REGTYPE_XMM:
			StringCchCopy(pwszOprStr, dwOprStrCount, awszXMMRegCodes[bReg]);
			break;

		default:
			wprintf_s(L"MODRM_GetOperandFromReg(): Invalid bRegType %u\n", bRegType);
			return OPRTYPE_ERROR;

	}// switch(bRegType)

	return OPRTYPE_REG;
}


BOOL MODRM_fSetPtrStr(PDASMSTATE pstState, BYTE bOperandSize, __out WCHAR *pwszPtrStr, DWORD dwPtrStrCount)
{
	ASSERT(bOperandSize >= OPERANDSIZE_UNDEF && bOperandSize <= OPERANDSIZE_64BIT);

	if(bOperandSize != OPERANDSIZE_UNDEF)
	{
		// Operand size already determined earlier;
		// maybe a fSplInstruction!
		StringCchCopy(pwszPtrStr, dwPtrStrCount, awszPtrStr[bOperandSize]);
		return TRUE;
	}

	// If bOperandSize has not been determined earlier, then we must
	// determine it using wBit and opsize attrib
	if(pstState->insCurIns.bWBit == 0)
		StringCchCopy(pwszPtrStr, dwPtrStrCount, awszPtrStr[PTR_STR_INDEX_BYTE]);
	else if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
		// 16bit
		StringCchCopy(pwszPtrStr, dwPtrStrCount, awszPtrStr[PTR_STR_INDEX_WORD]);
	else
		// 32bit
		StringCchCopy(pwszPtrStr, dwPtrStrCount, awszPtrStr[PTR_STR_INDEX_DWORD]);

	// todo: check ret val from StringCchCopy

	return TRUE;
}

/*
 * MODRM_bGetOperandSize()
 *	Returns the operand size based on the value of wBit and
 *	opsize attrib. This function must be NOT be used for
 *	fSplInstructions!
 */
BYTE MODRM_bGetOperandSize(PDASMSTATE pstState)
{
	if(!pstState->insCurIns.fWBitAbsent && pstState->insCurIns.bWBit == 0)
		return OPERANDSIZE_8BIT;
	
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
		return OPERANDSIZE_16BIT;

	return OPERANDSIZE_32BIT;
}

/*
 * *************************
 *		Begin SIB state	
 * *************************
 */

/* fStateSIB()
 * This state processes the SIB byte in the instruction.
 *
 * Args:
 *			
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fStateSIB(PDASMSTATE pstState)
{
	BYTE bSIB, bScale, bIndex, bBase;

	bSIB = *pstState->pbCurrentCodePtr;
	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;
	pstState->insCurIns.bSIB = bSIB;

	#ifdef _DEBUG
		wprintf_s(L"fStateSIB(): %X\n", bSIB);
	#endif

	// SplitModRM can be used because the structure of both
	// ModRM and SIB byte is the same.
	Util_vSplitModRMByte(bSIB, &bScale, &bIndex, &bBase);

	// _DEBUG only
	ASSERT(bScale >=0 && bScale <= 3);
	ASSERT(bIndex >= 0 && bIndex <= 7);
	ASSERT(bBase >= 0 && bBase <= 7);

	//												   I   S    B
	//	Eg.: 89 04 95 D0 60 D5 00	mov		dword ptr [edx*4+0D560D0h],eax
	// Above, B=base=disp32
	//

	// Determine the scale index string
	switch(bScale)
	{
		case 0:	// no scaling
			if(bIndex == 4)		// no index either
			{
				pstState->insCurIns.wszScaleIndex[0] = 0;	// must not print this in DUMP
				break;
			}

			// todo: Add '[' ']' in DUMP state: done
			StringCchPrintf(pstState->insCurIns.wszScaleIndex, _countof(pstState->insCurIns.wszScaleIndex), L"%s",
							awszRegCodes32[bIndex]);
			break;

		case 1:	// scale 2
			if(bIndex == 4)		// Nothing
			{
				pstState->insCurIns.wszScaleIndex[0] = 0;	// must not print this in DUMP
				break;
			}

			StringCchPrintf(pstState->insCurIns.wszScaleIndex, _countof(pstState->insCurIns.wszScaleIndex), L"%s*2",
							awszRegCodes32[bIndex]);
			break;

		case 2:	// scale 4
			if(bIndex == 4)		// Nothing
			{
				pstState->insCurIns.wszScaleIndex[0] = 0;	// must not print this in DUMP
				break;
			}

			StringCchPrintf(pstState->insCurIns.wszScaleIndex, _countof(pstState->insCurIns.wszScaleIndex), L"%s*4",
							awszRegCodes32[bIndex]);
			break;

		case 3:	// scale 8
			if(bIndex == 4)		// Nothing
			{
				pstState->insCurIns.wszScaleIndex[0] = 0;	// must not print this in DUMP
				break;
			}

			StringCchPrintf(pstState->insCurIns.wszScaleIndex, _countof(pstState->insCurIns.wszScaleIndex), L"%s*8",
							awszRegCodes32[bIndex]);
			break;

		default:	// We generally wouldn't reach here.
			wprintf_s(L"fStateSIB(): Invalid bIndex %d\n", bIndex);
			return FALSE;

	}// switch(bScale)

	// Determine base
	if(bBase == 5)
	{
		BYTE bMod;

		/* *****************************************************************************
		 *	NOTE: Table 2-3 IASD Vol-2
		 *	1. The [*] nomenclature(base=5) means a disp32 with no base if MOD is 00, 
		 *  [EBP] otherwise. This provides the following addressing modes:
		 *	disp32[index] (MOD=00).
		 *	disp8[EBP][index] (MOD=01).
		 *	disp32[EBP][index] (MOD=10).
		 * *****************************************************************************/
		
		// Get Mod field of ModRM byte: pstState->pbCurrentCodePtr-2 because we have moved past
		// the SIB byte by now.
		Util_vSplitModRMByte(*(pstState->pbCurrentCodePtr-2), &bMod, NULL, NULL);
		ASSERT(bMod >= 0 && bMod <= 2);	// ASSERT always??
		if(bMod == 0)
		{
			pstState->insCurIns.wszBase[0] = 0;
			pstState->insCurIns.bDispType = DISP_32BIT;
		}
		else
		{	// ebp is base; disp type has been set in MODRM already
			StringCchCopy(pstState->insCurIns.wszBase, _countof(pstState->insCurIns.wszBase), awszRegCodes32[REGCODE_EBP]);
			//if(bMod == 1)
			//	pstState->insCurIns.bDispType = DISP_8BIT;
			//else
			//	pstState->insCurIns.bDispType = DISP_32BIT;
		}
		pstState->insCurIns.fDisp = TRUE;
		pstState->dsNextState = DASM_STATE_DISP;
	}
	else
	{
		// todo: return value
		StringCchCopy(pstState->insCurIns.wszBase, _countof(pstState->insCurIns.wszBase), awszRegCodes32[bBase]);

		if(pstState->insCurIns.fDisp)
			// No need to determine size of disp here; done earlier.
			pstState->dsNextState = DASM_STATE_DISP;
		else if(pstState->insCurIns.fImm)
			// No need to determine size of imm here; done earlier.
			pstState->dsNextState = DASM_STATE_IMM;
		else
			pstState->dsNextState = DASM_STATE_DUMP;
	}

	return TRUE;

}// fStateSIB()


/*
 * *************************
 *		Begin DISP state	
 * *************************
 */

/* fStateDisp()
 * This state processes the Displacement byte in the instruction.
 *
 * Args:
 *			
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fStateDisp(PDASMSTATE pstState)
{
	// pstState->pbCurrentCodePtr must now be pointing to the disp byte
	// pstState->insCurIns.bDispType tells us the size of disp to read

	ASSERT(pstState->insCurIns.bDispType == DISP_8BIT 
			|| pstState->insCurIns.bDispType == DISP_32BIT);

	if(pstState->insCurIns.bDispType == DISP_8BIT)
	{
		pstState->insCurIns.iDisp = (char)*pstState->pbCurrentCodePtr;
		++pstState->pbCurrentCodePtr;
		++pstState->nBytesCurIns;
	}
	else if(pstState->insCurIns.bDispType == DISP_32BIT)
	{
		pstState->insCurIns.iDisp = (INT)*((INT*)pstState->pbCurrentCodePtr);
		pstState->pbCurrentCodePtr += sizeof(INT);
		pstState->nBytesCurIns += sizeof(INT);
	}

	if(pstState->insCurIns.fImm)
	{
		pstState->dsNextState = DASM_STATE_IMM;
		#ifdef _DEBUG
			if(pstState->insCurIns.bImmType == IMM_UNDEF)
			{
				wprintf_s(L"fStateDisp(): bImmType UNDEF\n");
				return FALSE;
			}
		#endif
		return TRUE;
	}

	// No imm byte: no other byte to process for current instruction
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;

}// fStateDisp()


/*
 * *************************
 *		Begin IMM state	
 * *************************
 */

/* fStateImm()
 * This state processes the Immediate value byte in the instruction.
 *
 * Args:
 *			
 * RetVal:
 *		TRUE/FALSE depending on whether there was an error or not.
 */
BOOL fStateImm(PDASMSTATE pstState)
{

	// Read the bImmType size of data from pstState->pbCurrentCodePtr

	if(pstState->insCurIns.fCodeOffset)
	{
		// Offset is added to the EIP, which is nothing but the 
		// value of pstState->pbCurrentCodePtr after reading the offset value
		// because pstState->pbCurrentCodePtr will then be pointing to the next
		// instruction.

		// Offset is a signed value

		// Store the offset value in pstState->insCurIns.iImm and let DUMP take care of 
		// calculating the actual address

		if(pstState->insCurIns.bImmType == IMM_8BIT)
		{
			// Do not write pstState->insCurIns.iImm = (INT)*pstState->pbCurrentCodePtr : causes compiler to emit
			// a movzx instruction but we need a movsx really
			pstState->insCurIns.iImm = (INT)(CHAR)(*pstState->pbCurrentCodePtr);
			++pstState->pbCurrentCodePtr;
			++pstState->nBytesCurIns;
		}
		else if(pstState->insCurIns.bImmType == IMM_16BIT)
		{
			pstState->insCurIns.iImm = (SHORT)(*(SHORT*)pstState->pbCurrentCodePtr);
			pstState->pbCurrentCodePtr += sizeof(SHORT);
			pstState->nBytesCurIns += sizeof(SHORT);
		}
		else if(pstState->insCurIns.bImmType == IMM_32BIT)
		{
			pstState->insCurIns.iImm = (INT)(*(INT*)pstState->pbCurrentCodePtr);
			pstState->pbCurrentCodePtr += sizeof(INT);
			pstState->nBytesCurIns += sizeof(INT);
		}
		else
		{
			wprintf_s(L"fStateImm(): Invalid code offset size %u\n", pstState->insCurIns.bImmType);
			return FALSE;
		}

		pstState->dsNextState = DASM_STATE_DUMP;
		return TRUE;
	}

	if(pstState->insCurIns.fDataOffset)
	{
		// moffs8/moffs16/moffs32

		// The data offset encoded as an immediate value can be either
		// 16/32 bits.
		if(pstState->insCurIns.bImmType == IMM_16BIT)
		{
			pstState->insCurIns.iImm = (SHORT)(*(SHORT*)pstState->pbCurrentCodePtr);
			pstState->pbCurrentCodePtr += sizeof(SHORT);
			pstState->nBytesCurIns += sizeof(SHORT);
		}
		else if(pstState->insCurIns.bImmType == IMM_32BIT)
		{
			pstState->insCurIns.iImm = (INT)(*(INT*)pstState->pbCurrentCodePtr);
			pstState->pbCurrentCodePtr += sizeof(INT);
			pstState->nBytesCurIns += sizeof(INT);
		}
		else
		{
			wprintf_s(L"fStateImm(): Invalid data offset size %u\n", pstState->insCurIns.bImmType);
			return FALSE;
		}

		pstState->dsNextState = DASM_STATE_DUMP;
		return TRUE;
	}// if fDataOffset

	// Not a code/data offset, just an immediate value, which is signed(!).
	// Store into pstState->insCurIns.iImm and let DUMP take care of printing the value.
	if(pstState->insCurIns.bImmType == IMM_8BIT)
	{
		if(pstState->insCurIns.fImmSignEx)
			pstState->insCurIns.iImm = (INT)(CHAR)*pstState->pbCurrentCodePtr;
		else
			pstState->insCurIns.iImm = (INT)*pstState->pbCurrentCodePtr;
		++pstState->pbCurrentCodePtr;
		++pstState->nBytesCurIns;
	}
	else if(pstState->insCurIns.bImmType == IMM_16BIT)
	{
		pstState->insCurIns.iImm = (SHORT)(*(SHORT*)pstState->pbCurrentCodePtr);
		pstState->pbCurrentCodePtr += sizeof(SHORT);
		pstState->nBytesCurIns += sizeof(SHORT);
	}
	else if(pstState->insCurIns.bImmType == IMM_32BIT)
	{
		pstState->insCurIns.iImm = (INT)(*(INT*)pstState->pbCurrentCodePtr);
		pstState->pbCurrentCodePtr += sizeof(INT);
		pstState->nBytesCurIns += sizeof(INT);
	}
	else
	{
		wprintf_s(L"fStateImm(): Invalid bImmType %d\n", pstState->insCurIns.bImmType);
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_DUMP;

	return TRUE;
}// fStateImm()


/*
 * *************************
 *		Begin DUMP state
 * *************************
 */

BOOL fStateDumpOnOpcodeError(PDASMSTATE pstState)
{
	// 1: address
	wprintf_s(L"  %08X: ", (pstState->pbCurrentCodePtr-pstState->nBytesCurIns)+pstState->lDelta);
	wprintf_s(L"%02X\n", *(pstState->pbCurrentCodePtr-pstState->nBytesCurIns));
	return TRUE;
}

BOOL fStateDumpOnOpcodeError_ToString(PDASMSTATE pstState)
{
    DWORD nTotalCharsWritten = 0;
    DWORD nTotalCharsCapacity = _countof(pstState->szDisassembledInst);
    PWCHAR pszStrStart;

    pszStrStart = pstState->szDisassembledInst;

	// 1: address
	nTotalCharsWritten += swprintf_s(
                            pszStrStart, 
                            nTotalCharsCapacity - nTotalCharsWritten, 
                            L"  %08X: ", (pstState->pbCurrentCodePtr-pstState->nBytesCurIns)+pstState->lDelta);

	swprintf_s(
        pszStrStart + nTotalCharsWritten, 
        nTotalCharsCapacity - nTotalCharsWritten, 
        L"%02X\n", *(pstState->pbCurrentCodePtr-pstState->nBytesCurIns));
	
    return TRUE;
}

BOOL fStateDump(PDASMSTATE pstState)
{
	static WCHAR awszPrefixStr[][8] = { L"lock", L"repne", L"repe" };
	static INT aiPrefixStrNChars[] = { 4, 5, 4 };

	WCHAR *pwszSegStr = NULL;	// segment register, if any
	
	register INT nBytesPrinted = 0;
	INT nCharsMnemPrinted = 0;

	// Eg.: 0040106F: 68 AD DE 00 00		push        0DEADh

	// Eg.:	0040108F: 89 04 95 D0 60 D5 00	mov			dword ptr [edx*4+0D560D0h],eax

	// 1: address
	wprintf_s(L"  %08X: ", (pstState->pbCurrentCodePtr-pstState->nBytesCurIns)+pstState->lDelta);

	// 2: Bytes that make up the current instruction
	// Print the first 6 bytes on the current line; if there are more
	// bytes remaining, then print them on the next line, later.
	for(PBYTE pBytes = pstState->pbCurrentCodePtr - pstState->nBytesCurIns;
		pBytes < pstState->pbCurrentCodePtr && nBytesPrinted < 6; 
		++pBytes, ++nBytesPrinted)
	{
		wprintf_s(L"%02X ", *pBytes);
	}

	// Padding if there are < 6 bytes in the current instruction
	// 3 space chars in each iteration because each byte printed
	// occupies 3spaces. Eg.: "88 8C 05 "
	for(INT nSpaces = pstState->nBytesCurIns; 
		nSpaces < 6; ++nSpaces)
	{
		wprintf_s(L"   ");
	}

	/*
	 * Keeping track of the number of characters as part of the
	 * instruction mnemonic so that the correct amount of padding
	 * can be printed and sp the operands are properly aligned 
	 * in every instruction. Instruction mnemonic includes prefixes
	 * and the opcode mnemonic itself.
	 */

	// 3: Print LOCK, REPx prefixes, if any
	if(pstState->insCurIns.wPrefixTypes & PREFIX_LOCKREP)
	{
		switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP])
		{
			case 0xf0:	// lock
				wprintf_s(L"%s ", awszPrefixStr[0]);
				nCharsMnemPrinted += aiPrefixStrNChars[0] + 1;	// +1 for a space char
				break;

			case 0xf2:	// repne
				if(pstState->insCurIns.fSSEIns)	// no prefix if SSE prefix
					break;
				wprintf_s(L"%s ", awszPrefixStr[1]);
				nCharsMnemPrinted += aiPrefixStrNChars[1] + 1;
				break;

			case 0xf3:	// repe
				if(pstState->insCurIns.fSSEIns)	// no prefix if SSE prefix
					break;
				wprintf_s(L"%s ", awszPrefixStr[2]);
				nCharsMnemPrinted += aiPrefixStrNChars[2] + 1;
				break;

			default:
				#ifdef _DEBUG
					wprintf_s(L"fStateDump(): Invalid prefix %d\n",
								pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP]);
					break;
				#else
					break;
				#endif
		}// switch()
	
	}// if there is a lock/rep prefix

	// 4: Instruction mnemonic
	wprintf_s(L"%s", pstState->wszCurInsStr);
	nCharsMnemPrinted += wcslen(pstState->wszCurInsStr);

	// Now print padding spaces if required
	for(register INT nPad = nCharsMnemPrinted;
		nPad <= NUM_CHARS_MNEMONIC;
		++nPad)
	{
		wprintf_s(L" ");
	}

	// 5: Operands
	if(pstState->insCurIns.fModRM)
	{
		// Destination operand first
		switch(pstState->insCurIns.OprTypeDes)
		{
			case OPRTYPE_ERROR:
				// Instructions that do not have a ModRM/SIB byte would not
				// set the OprTypeDes and it will be OPRTYPE_ERROR(=0).
				// Or have a /digit ModRM byte, in which case it specifies only
				// one operand and that is in the src str.
				break;

			case OPRTYPE_REG:	// eax, ax, al, ...
				wprintf_s(L"%s", pstState->insCurIns.wszCurInsStrDes);
				break;

			case OPRTYPE_MEM:	// dword ptr [eax], word ptr [ebx], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				wprintf_s(L"%s%s[%s]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
							pstState->insCurIns.wszCurInsStrDes);
				break;
			}

			case OPRTYPE_MEM8:	// dword ptr [eax-4h], word ptr [ebx+10h], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				if(pstState->insCurIns.iDisp >= 0)	// must include a '+'
					wprintf_s(L"%s%s[%s+%02Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
								pstState->insCurIns.wszCurInsStrDes, pstState->insCurIns.iDisp);
				else
				{
					// Get the positive representation of the negative displacement and put a 
					// '-' sign preceding it while printing. This makes it easier to spot
					// positive and negative displacements (especially from ebp: diff b/w
					// locals and arguments).
					BYTE chDisp = (BYTE)pstState->insCurIns.iDisp;
					Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
					wprintf_s(L"%s%s[%s-%02Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
								pstState->insCurIns.wszCurInsStrDes, chDisp);
				}
				break;
			}// case MEM8

			case OPRTYPE_MEM32:
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

				// dumpbin appears to be doing it(2's compl thing) only for disp8, not disp32
				wprintf_s(L"%s%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
							pstState->insCurIns.wszCurInsStrDes, pstState->insCurIns.iDisp);
				break;
			}// case MEM32

			/* Note:
			 * There may be a displacement when Mod=0; although MODRM state does not know this,
			 * SIB state does and sets fDisp=TRUE. So yes, we can combine all SIB oprtypes.
			 */
			case OPRTYPE_SIB8:
			{
				if(pstState->insCurIns.fDisp)
				{
					BYTE chDisp;

					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// only a disp
							wprintf_s(L"%s[%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iDisp);
						else	// scaled-index+disp
						{
							if(pstState->insCurIns.iDisp >= 0)
								wprintf_s(L"%s[%s+%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
							else
							{
								chDisp = (BYTE)pstState->insCurIns.iDisp;
								Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
								wprintf_s(L"%s[%s-%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, chDisp);
							}
						}
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// base + disp
						{
							if(pstState->insCurIns.iDisp >= 0)
								wprintf_s(L"%s[%s+%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.iDisp);
							else
							{
								chDisp = (BYTE)pstState->insCurIns.iDisp;
								Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
								wprintf_s(L"%s[%s-%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, chDisp);
							}
						}
						else	// base+scaled-index+disp
						{
							if(pstState->insCurIns.iDisp >= 0)
								wprintf_s(L"%s[%s+%s+%02Xh]", pstState->insCurIns.pwszPtrStr,
										pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
							else
							{
								chDisp = (BYTE)pstState->insCurIns.iDisp;
								Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
								wprintf_s(L"%s[%s+%s-%02Xh]", pstState->insCurIns.pwszPtrStr,
										pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, chDisp);
							}
						}
					}// if base
				}
				else
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: no scale, no index, no base!! error??
						{
							wprintf_s(L"fStateDump(): SIB no base, no SI, no disp!!\n");
							return FALSE;
						}
						else	// scaled-index
							wprintf_s(L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: only base??
							wprintf_s(L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase);
						else	// base + scaled-index
							wprintf_s(L"%s[%s+%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex);
					}
				}// if fDisp

				break;
			}// SIB8

			case OPRTYPE_SIB:
			case OPRTYPE_SIB32:
			{
				if(pstState->insCurIns.fDisp)
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// only a disp
							wprintf_s(L"%s[%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iDisp);
						else	// scaled-index+disp
							wprintf_s(L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// base + disp
							wprintf_s(L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.iDisp);
						else	// base+scaled-index+disp
							wprintf_s(L"%s[%s+%s+%Xh]", pstState->insCurIns.pwszPtrStr,
									pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
				}
				else
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: no scale, no index, no base!! error??
						{
							wprintf_s(L"fStateDump(): SIB no base, no SI, no disp!!\n");
							return FALSE;
						}
						else	// scaled-index
							wprintf_s(L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: only base??
							wprintf_s(L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase);
						else	// base + scaled-index
							wprintf_s(L"%s[%s+%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex);
					}
				}// if fDisp

				break;
			}// case SIB/SIB32

			case OPRTYPE_DISP32:	// ds:[disp32]	ds:[0040BD08h]
			{
				if(pstState->insCurIns.wPrefixTypes & PREFIX_SEG)
				{
					// We have a segment override prefix. Get it.
					DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

					wprintf_s(L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, pstState->insCurIns.iDisp);
				}
				else
				{
					// Default to "ds:" if there is no segment override prefix
					wprintf_s(L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				}

				//switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
				//{
				//	case OPC_PREFIX_SEGOVR_ES:
				//		wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_CS:
				//		wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_SS:
				//		wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_DS:
				//		wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_FS:
				//		wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_GS:
				//		wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iDisp);
				//		break;

				//	default:	// redundant but makes it easy to understand
				//		wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				//		break;
				//}
			}// case OPRTYPE_DISP32

			default:
				break;

		}// switch(OprTypeDes)

		// ',' between dest and src operands only if both are present
		if(pstState->insCurIns.fDesStrSet && pstState->insCurIns.fSrcStrSet)
			wprintf_s(L",");

		// Now the source operand
		switch(pstState->insCurIns.OprTypeSrc)
		{
			case OPRTYPE_ERROR:
				// Instructions that do not have a ModRM/SIB byte would not
				// set the OprTypeDes and it will be OPRTYPE_ERROR(=0).
				break;

			case OPRTYPE_REG:	// eax, ax, al, ...
				wprintf_s(L"%s", pstState->insCurIns.wszCurInsStrSrc);
				break;

			case OPRTYPE_MEM:	// dword ptr [eax], word ptr [ebx], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				wprintf_s(L"%s%s[%s]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
							pstState->insCurIns.wszCurInsStrSrc);
				break;
			}

			case OPRTYPE_MEM8:	// dword ptr [eax-4h], word ptr [ebx+10h], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				if(pstState->insCurIns.iDisp >= 0)	// must include a '+'
					wprintf_s(L"%s%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
								pstState->insCurIns.wszCurInsStrSrc, pstState->insCurIns.iDisp);
				else
				{
					// Get the positive representation of the negative displacement and put a 
					// '-' sign preceding it while printing. This makes it easier to spot
					// positive and negative displacements (especially from ebp: diff b/w
					// locals and arguments).
					BYTE chDisp = (BYTE)pstState->insCurIns.iDisp;
					Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
					wprintf_s(L"%s%s[%s-%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
								pstState->insCurIns.wszCurInsStrSrc, chDisp);
				}
				break;
			}// case MEM8

			case OPRTYPE_MEM32:
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

				// dumpbin appears to be doing it(2's compl thing) only for disp8, not disp32
				wprintf_s(L"%s%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
							pstState->insCurIns.wszCurInsStrSrc, pstState->insCurIns.iDisp);
				break;
			}// case MEM32

			case OPRTYPE_SIB:
			case OPRTYPE_SIB8:
			case OPRTYPE_SIB32:
			{
				if(pstState->insCurIns.fDisp)
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// only a disp
							wprintf_s(L"%s[%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iDisp);
						else	// scaled-index+disp
							wprintf_s(L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// base + disp
							wprintf_s(L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.iDisp);
						else	// base+scaled-index+disp
							wprintf_s(L"%s[%s+%s+%Xh]", pstState->insCurIns.pwszPtrStr,
									pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
				}
				else
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: no scale, no index, no base!! error??
						{
							wprintf_s(L"fStateDump(): SIB no base, no SI, no disp!!\n");
							return FALSE;
						}
						else	// scaled-index
							wprintf_s(L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: only base??
							wprintf_s(L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase);
						else	// base + scaled-index
							wprintf_s(L"%s[%s+%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex);
					}
				}// if fDisp

				break;
			}// case SIB8/32

			case OPRTYPE_DISP32:	// ds:[disp32]	ds:[0040BD08h]
			{
				WCHAR *pwszSegStr = NULL;

				if(pstState->insCurIns.wPrefixTypes & PREFIX_SEG)
				{
					// We have a segment override prefix. Get it.
					DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

					wprintf_s(L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, pstState->insCurIns.iDisp);
				}
				else
				{
					// Default to "ds:" if there is no segment override prefix
					wprintf_s(L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				}
				

			//	switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
			//	{
			//		case OPC_PREFIX_SEGOVR_ES:
			//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_CS:
			//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_SS:
			//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_DS:
			//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_FS:
			//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_GS:
			//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iDisp);
			//			break;

			//		default:	// redundant but makes it easy to understand
			//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
			//			break;
			//	}
			}// case OPRTYPE_DISP32

			default:
				break;

		}// switch(OprTypeDes)

	}
	else
	{
		// There was no ModRM byte to specify the operand(s).
		// Options:
		// - Operand(s) are determined in OPCODE state
		// - Data offset is present (src/des)
		// - Imm value is src
		// - Code offset is src

		if(pstState->insCurIns.fDesStrSet)
			wprintf_s(L"%s", pstState->insCurIns.wszCurInsStrDes);
		else if(pstState->insCurIns.fDataOffset && pstState->insCurIns.fSrcStrSet)
		{
			// des not determined earlier but src has been determined
			// and there is a data offset. So the data offset must be
			// the des operand
			DUMP_vDumpDataOffset(pstState);
			pstState->insCurIns.fDesStrSet = TRUE;	// we have a des operand: print ',' later
		}

		if(pstState->insCurIns.fSrcStrSet)
		{
			// ',' between dest and src operands only if both are present
			if(pstState->insCurIns.fDesStrSet)
				wprintf_s(L",");
			wprintf_s(L"%s", pstState->insCurIns.wszCurInsStrSrc);
		}
		else if(pstState->insCurIns.fDataOffset)
		{
			// data offset must be the src operand
			if(pstState->insCurIns.fDesStrSet)
				wprintf_s(L",");
			DUMP_vDumpDataOffset(pstState);
			pstState->insCurIns.fSrcStrSet = TRUE;	// we have a src operand: print ',' later
		}

	}// if fModRM

	// Now the third operand
	if(pstState->insCurIns.fOpr3StrSet)
		wprintf_s(L",%s", pstState->insCurIns.wszCurInsStrOpr3);

	// 6: Immediate value: imm or code or data offset
	if(pstState->insCurIns.fImm)
	{									//  ud   8bit       16bit     32bit     48bit
 		static WCHAR awszFormatSpec[][6] = {L"", L"%02Xh", L"%04Xh", L"%08Xh", L"%Xh"};
		
		// todo: ASSERT in Release mode too??
		ASSERT(pstState->insCurIns.bImmType >= IMM_UNDEF && pstState->insCurIns.bImmType <= IMM_48BIT);	// Array bounds checking

		// Preceding comma(',') if there was a prior operand
		if(pstState->insCurIns.fDesStrSet || pstState->insCurIns.fSrcStrSet)	// checking src also because /digit type
			wprintf_s(L",");								// sets oprstr in src

		wprintf_s(awszFormatSpec[pstState->insCurIns.bImmType], pstState->insCurIns.iImm);
	}
	else if(pstState->insCurIns.fCodeOffset)
	{
		// Preceding comma(',') if there was a prior operand
		if(pstState->insCurIns.fDesStrSet || pstState->insCurIns.fSrcStrSet)	// checking src also because /digit type
			wprintf_s(L",");								// sets oprstr in src

		LONG lTemp = pstState->lDelta + (LONG)pstState->pbCurrentCodePtr;

		// 32bit address always; so %08X
		wprintf_s(L"%08X", pstState->insCurIns.iImm + lTemp);
	}
	//else if(pstState->insCurIns.fDataOffset)
	//{
	//	if(pstState->insCurIns.fSrcStrSet || pstState->insCurIns.fDesStrSet)	// checking both because A3 C4 ... MOV eax,
	//		wprintf_s(L",");

	//	/*
	//	 * By default, the offset is in the ds segment but this maybe
	//	 * changed using any one of the segment override prefixes.
	//	 * Sample string:
	//	 *		dword ptr ds:[0040B000h]
	//	 *		word ptr ss:[0040B000h]
	//	 */
	//	 if( ! (pstState->insCurIns.wPrefixTypes & PREFIX_SEG) )	// ! higher precedence than &
	//	 {
	//		// No segment override prefix present, default to ds
	//		if(pstState->insCurIns.bImmType == IMM_16BIT)
	//			wprintf_s(L"%s ds:[%04Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	//		else
	//			wprintf_s(L"%s ds:[%08Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	//	 }
	//	 else
	//	 {
	//		// There is a segment override prefix. Print the correct one
	//		// by reading the stored prefix type in abPrefixes.
	//		switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
	//		{
	//			case OPC_PREFIX_SEGOVR_ES:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				else
	//					wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				break;

	//			case OPC_PREFIX_SEGOVR_CS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
	//				else
	//					wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
	//				break;

	//			case OPC_PREFIX_SEGOVR_SS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
	//				else
	//					wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
	//				break;

	//			case OPC_PREFIX_SEGOVR_DS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
	//				else
	//					wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
	//				break;
	//			
	//			case OPC_PREFIX_SEGOVR_FS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
	//				else
	//					wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
	//				break;
	//			
	//			case OPC_PREFIX_SEGOVR_GS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
	//				else
	//					wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
	//				break;

	//			default:	// Not a critical error
	//				wprintf_s(L"fStateDump(): Invalid segovr prefix %xh\n", pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG]);
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					wprintf_s(L"%s <>:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				else
	//					wprintf_s(L"%s <>:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				break;
	//		}// switch(abPrefixes)
	//	 
	//	 }// if PREFIX_SEG
	//}// if fDataOffset

	// 7: Go to next line
	wprintf_s(L"\n");

	// 8: Check if there are more bytes of the current instruction
	// to print.
	if(pstState->nBytesCurIns > 6)
	{
		// In the earlier print loop, nBytesPrinted is incremented after 
		// printing and before checking termination condition, so 
		// its value maybe greater than how many bytes were actually printed
		nBytesPrinted = 6;

		// Padding for address field
		wprintf_s(L"            ");	// 12 spaces

		// Start from where we left off earlier
		register int nNewPrinted = 0;
		for(PBYTE pBytes = pstState->pbCurrentCodePtr - pstState->nBytesCurIns + nBytesPrinted;
		pBytes < pstState->pbCurrentCodePtr && nNewPrinted < pstState->nBytesCurIns;
		++pBytes, ++nNewPrinted)
		{
			wprintf_s(L"%02X ", *pBytes);
		}
		wprintf_s(L"\n");
	}

	pstState->dsNextState = DASM_STATE_RESET;
	return TRUE;

}

BOOL fStateDump_ToString(PDASMSTATE pstState)
{
	static WCHAR awszPrefixStr[][8] = { L"lock", L"repne", L"repe" };
	static INT aiPrefixStrNChars[] = { 4, 5, 4 };

	WCHAR *pwszSegStr = NULL;	// segment register, if any
	
	register INT nBytesPrinted = 0;
	INT nCharsMnemPrinted = 0;

    PWCHAR pszStrStart;
    DWORD nTotalCharsCapacity;
    DWORD nTotalCharsWritten;

	// Eg.: 0040106F: 68 AD DE 00 00		push        0DEADh

	// Eg.:	0040108F: 89 04 95 D0 60 D5 00	mov			dword ptr [edx*4+0D560D0h],eax

    // Init
    pszStrStart = pstState->szDisassembledInst;
    nTotalCharsCapacity = _countof(pstState->szDisassembledInst);
    nTotalCharsWritten = 0;

	// 1: address
    nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"  %08X: ", (pstState->pbCurrentCodePtr - pstState->nBytesCurIns)+pstState->lDelta);

	// 2: Bytes that make up the current instruction
	// Print the first 6 bytes on the current line; if there are more
	// bytes remaining, then print them on the next line, later.
	for(PBYTE pBytes = pstState->pbCurrentCodePtr - pstState->nBytesCurIns;
		pBytes < pstState->pbCurrentCodePtr && nBytesPrinted < 6; 
		++pBytes, ++nBytesPrinted)
	{
		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%02X ", *pBytes);
	}

	// Padding if there are < 6 bytes in the current instruction
	// 3 space chars in each iteration because each byte printed
	// occupies 3spaces. Eg.: "88 8C 05 "
	for(INT nSpaces = pstState->nBytesCurIns; 
		nSpaces < 6; ++nSpaces)
	{
		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"   ");
	}

	/*
	 * Keeping track of the number of characters as part of the
	 * instruction mnemonic so that the correct amount of padding
	 * can be printed and sp the operands are properly aligned 
	 * in every instruction. Instruction mnemonic includes prefixes
	 * and the opcode mnemonic itself.
	 */

	// 3: Print LOCK, REPx prefixes, if any
	if(pstState->insCurIns.wPrefixTypes & PREFIX_LOCKREP)
	{
		switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP])
		{
			case 0xf0:	// lock
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s ", awszPrefixStr[0]);
				nCharsMnemPrinted += aiPrefixStrNChars[0] + 1;	// +1 for a space char
				break;

			case 0xf2:	// repne
				if(pstState->insCurIns.fSSEIns)	// no prefix if SSE prefix
					break;
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s ", awszPrefixStr[1]);
				nCharsMnemPrinted += aiPrefixStrNChars[1] + 1;
				break;

			case 0xf3:	// repe
				if(pstState->insCurIns.fSSEIns)	// no prefix if SSE prefix
					break;
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s ", awszPrefixStr[2]);
				nCharsMnemPrinted += aiPrefixStrNChars[2] + 1;
				break;

			default:
                    ASSERT(FALSE);
					dbgwprintf(L"fStateDump(): Invalid prefix %d\n",
								pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP]);
					break;
		}// switch()
	
	}// if there is a lock/rep prefix

	// 4: Instruction mnemonic
	nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s", pstState->wszCurInsStr);
	nCharsMnemPrinted += wcslen(pstState->wszCurInsStr);

	// Now print padding spaces if required
	for(register INT nPad = nCharsMnemPrinted;
		nPad <= NUM_CHARS_MNEMONIC;
		++nPad)
	{
		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L" ");
	}

	// 5: Operands
	if(pstState->insCurIns.fModRM)
	{
		// Destination operand first
		switch(pstState->insCurIns.OprTypeDes)
		{
			case OPRTYPE_ERROR:
				// Instructions that do not have a ModRM/SIB byte would not
				// set the OprTypeDes and it will be OPRTYPE_ERROR(=0).
				// Or have a /digit ModRM byte, in which case it specifies only
				// one operand and that is in the src str.
				break;

			case OPRTYPE_REG:	// eax, ax, al, ...
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s", pstState->insCurIns.wszCurInsStrDes);
				break;

			case OPRTYPE_MEM:	// dword ptr [eax], word ptr [ebx], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
							pstState->insCurIns.wszCurInsStrDes);
				break;
			}

			case OPRTYPE_MEM8:	// dword ptr [eax-4h], word ptr [ebx+10h], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				if(pstState->insCurIns.iDisp >= 0)	// must include a '+'
					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s+%02Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
								pstState->insCurIns.wszCurInsStrDes, pstState->insCurIns.iDisp);
				else
				{
					// Get the positive representation of the negative displacement and put a 
					// '-' sign preceding it while printing. This makes it easier to spot
					// positive and negative displacements (especially from ebp: diff b/w
					// locals and arguments).
					BYTE chDisp = (BYTE)pstState->insCurIns.iDisp;
					Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s-%02Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
								pstState->insCurIns.wszCurInsStrDes, chDisp);
				}
				break;
			}// case MEM8

			case OPRTYPE_MEM32:
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

				// dumpbin appears to be doing it(2's compl thing) only for disp8, not disp32
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
							pstState->insCurIns.wszCurInsStrDes, pstState->insCurIns.iDisp);
				break;
			}// case MEM32

			/* Note:
			 * There may be a displacement when Mod=0; although MODRM state does not know this,
			 * SIB state does and sets fDisp=TRUE. So yes, we can combine all SIB oprtypes.
			 */
			case OPRTYPE_SIB8:
			{
				if(pstState->insCurIns.fDisp)
				{
					BYTE chDisp;

					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// only a disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iDisp);
						else	// scaled-index+disp
						{
							if(pstState->insCurIns.iDisp >= 0)
								nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
							else
							{
								chDisp = (BYTE)pstState->insCurIns.iDisp;
								Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
								nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s-%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, chDisp);
							}
						}
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// base + disp
						{
							if(pstState->insCurIns.iDisp >= 0)
								nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.iDisp);
							else
							{
								chDisp = (BYTE)pstState->insCurIns.iDisp;
								Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
								nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s-%02Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, chDisp);
							}
						}
						else	// base+scaled-index+disp
						{
							if(pstState->insCurIns.iDisp >= 0)
								nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%s+%02Xh]", pstState->insCurIns.pwszPtrStr,
										pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
							else
							{
								chDisp = (BYTE)pstState->insCurIns.iDisp;
								Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
								nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%s-%02Xh]", pstState->insCurIns.pwszPtrStr,
										pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, chDisp);
							}
						}
					}// if base
				}
				else
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: no scale, no index, no base!! error??
						{
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"fStateDump(): SIB no base, no SI, no disp!!\n");
							return FALSE;
						}
						else	// scaled-index
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: only base??
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase);
						else	// base + scaled-index
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex);
					}
				}// if fDisp

				break;
			}// SIB8

			case OPRTYPE_SIB:
			case OPRTYPE_SIB32:
			{
				if(pstState->insCurIns.fDisp)
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// only a disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iDisp);
						else	// scaled-index+disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// base + disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.iDisp);
						else	// base+scaled-index+disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%s+%Xh]", pstState->insCurIns.pwszPtrStr,
									pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
				}
				else
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: no scale, no index, no base!! error??
						{
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"fStateDump(): SIB no base, no SI, no disp!!\n");
							return FALSE;
						}
						else	// scaled-index
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: only base??
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase);
						else	// base + scaled-index
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex);
					}
				}// if fDisp

				break;
			}// case SIB/SIB32

			case OPRTYPE_DISP32:	// ds:[disp32]	ds:[0040BD08h]
			{
				if(pstState->insCurIns.wPrefixTypes & PREFIX_SEG)
				{
					// We have a segment override prefix. Get it.
					DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, pstState->insCurIns.iDisp);
				}
				else
				{
					// Default to "ds:" if there is no segment override prefix
					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				}

				//switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
				//{
				//	case OPC_PREFIX_SEGOVR_ES:
				//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_CS:
				//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_SS:
				//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_DS:
				//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_FS:
				//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iDisp);
				//		break;

				//	case OPC_PREFIX_SEGOVR_GS:
				//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iDisp);
				//		break;

				//	default:	// redundant but makes it easy to understand
				//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				//		break;
				//}
			}// case OPRTYPE_DISP32

			default:
				break;

		}// switch(OprTypeDes)

		// ',' between dest and src operands only if both are present
		if(pstState->insCurIns.fDesStrSet && pstState->insCurIns.fSrcStrSet)
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L",");

		// Now the source operand
		switch(pstState->insCurIns.OprTypeSrc)
		{
			case OPRTYPE_ERROR:
				// Instructions that do not have a ModRM/SIB byte would not
				// set the OprTypeDes and it will be OPRTYPE_ERROR(=0).
				break;

			case OPRTYPE_REG:	// eax, ax, al, ...
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s", pstState->insCurIns.wszCurInsStrSrc);
				break;

			case OPRTYPE_MEM:	// dword ptr [eax], word ptr [ebx], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s]", pstState->insCurIns.pwszPtrStr, pwszSegStr,
							pstState->insCurIns.wszCurInsStrSrc);
				break;
			}

			case OPRTYPE_MEM8:	// dword ptr [eax-4h], word ptr [ebx+10h], ...
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
				if(pstState->insCurIns.iDisp >= 0)	// must include a '+'
					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
								pstState->insCurIns.wszCurInsStrSrc, pstState->insCurIns.iDisp);
				else
				{
					// Get the positive representation of the negative displacement and put a 
					// '-' sign preceding it while printing. This makes it easier to spot
					// positive and negative displacements (especially from ebp: diff b/w
					// locals and arguments).
					BYTE chDisp = (BYTE)pstState->insCurIns.iDisp;
					Util_vTwosComplementByte(chDisp, &chDisp);		// same variable as two parameters I know!
					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s-%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
								pstState->insCurIns.wszCurInsStrSrc, chDisp);
				}
				break;
			}// case MEM8

			case OPRTYPE_MEM32:
			{
				DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

				// dumpbin appears to be doing it(2's compl thing) only for disp8, not disp32
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, 
							pstState->insCurIns.wszCurInsStrSrc, pstState->insCurIns.iDisp);
				break;
			}// case MEM32

			case OPRTYPE_SIB:
			case OPRTYPE_SIB8:
			case OPRTYPE_SIB32:
			{
				if(pstState->insCurIns.fDisp)
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// only a disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iDisp);
						else	// scaled-index+disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// base + disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.iDisp);
						else	// base+scaled-index+disp
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%s+%Xh]", pstState->insCurIns.pwszPtrStr,
									pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex, pstState->insCurIns.iDisp);
					}
				}
				else
				{
					if( pstState->insCurIns.wszBase[0] == 0 )
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: no scale, no index, no base!! error??
						{
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"fStateDump(): SIB no base, no SI, no disp!!\n");
							return FALSE;
						}
						else	// scaled-index
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszScaleIndex);
					}
					else
					{
						if(pstState->insCurIns.wszScaleIndex[0] == 0)	// todo: only base??
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase);
						else	// base + scaled-index
							nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s[%s+%s]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.wszBase, pstState->insCurIns.wszScaleIndex);
					}
				}// if fDisp

				break;
			}// case SIB8/32

			case OPRTYPE_DISP32:	// ds:[disp32]	ds:[0040BD08h]
			{
				WCHAR *pwszSegStr = NULL;

				if(pstState->insCurIns.wPrefixTypes & PREFIX_SEG)
				{
					// We have a segment override prefix. Get it.
					DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);

					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, pwszSegStr, pstState->insCurIns.iDisp);
				}
				else
				{
					// Default to "ds:" if there is no segment override prefix
					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
				}
				

			//	switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
			//	{
			//		case OPC_PREFIX_SEGOVR_ES:
			//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_CS:
			//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_SS:
			//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_DS:
			//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_FS:
			//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iDisp);
			//			break;

			//		case OPC_PREFIX_SEGOVR_GS:
			//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iDisp);
			//			break;

			//		default:	// redundant but makes it easy to understand
			//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iDisp);
			//			break;
			//	}
			}// case OPRTYPE_DISP32

			default:
				break;

		}// switch(OprTypeDes)

	}
	else
	{
		// There was no ModRM byte to specify the operand(s).
		// Options:
		// - Operand(s) are determined in OPCODE state
		// - Data offset is present (src/des)
		// - Imm value is src
		// - Code offset is src

		if(pstState->insCurIns.fDesStrSet)
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s", pstState->insCurIns.wszCurInsStrDes);
		else if(pstState->insCurIns.fDataOffset && pstState->insCurIns.fSrcStrSet)
		{
			// des not determined earlier but src has been determined
			// and there is a data offset. So the data offset must be
			// the des operand
			DUMP_vDumpDataOffset_ToString(pstState, &nTotalCharsWritten);
			pstState->insCurIns.fDesStrSet = TRUE;	// we have a des operand: print ',' later
		}

		if(pstState->insCurIns.fSrcStrSet)
		{
			// ',' between dest and src operands only if both are present
			if(pstState->insCurIns.fDesStrSet)
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L",");
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s", pstState->insCurIns.wszCurInsStrSrc);
		}
		else if(pstState->insCurIns.fDataOffset)
		{
			// data offset must be the src operand
			if(pstState->insCurIns.fDesStrSet)
				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L",");
			DUMP_vDumpDataOffset_ToString(pstState, &nTotalCharsWritten);
			pstState->insCurIns.fSrcStrSet = TRUE;	// we have a src operand: print ',' later
		}

	}// if fModRM

	// Now the third operand
	if(pstState->insCurIns.fOpr3StrSet)
		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L",%s", pstState->insCurIns.wszCurInsStrOpr3);

	// 6: Immediate value: imm or code or data offset
	if(pstState->insCurIns.fImm)
	{									//  ud   8bit       16bit     32bit     48bit
 		static WCHAR awszFormatSpec[][6] = {L"", L"%02Xh", L"%04Xh", L"%08Xh", L"%Xh"};
		
		// todo: ASSERT in Release mode too??
		ASSERT(pstState->insCurIns.bImmType >= IMM_UNDEF && pstState->insCurIns.bImmType <= IMM_48BIT);	// Array bounds checking

		// Preceding comma(',') if there was a prior operand
		if(pstState->insCurIns.fDesStrSet || pstState->insCurIns.fSrcStrSet)	// checking src also because /digit type
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L",");								// sets oprstr in src

		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, awszFormatSpec[pstState->insCurIns.bImmType], pstState->insCurIns.iImm);
	}
	else if(pstState->insCurIns.fCodeOffset)
	{
		// Preceding comma(',') if there was a prior operand
		if(pstState->insCurIns.fDesStrSet || pstState->insCurIns.fSrcStrSet)	// checking src also because /digit type
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L",");								// sets oprstr in src

		LONG lTemp = pstState->lDelta + (LONG)pstState->pbCurrentCodePtr;

		// 32bit address always; so %08X
		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%08X", pstState->insCurIns.iImm + lTemp);
	}
	//else if(pstState->insCurIns.fDataOffset)
	//{
	//	if(pstState->insCurIns.fSrcStrSet || pstState->insCurIns.fDesStrSet)	// checking both because A3 C4 ... MOV eax,
	//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L",");

	//	/*
	//	 * By default, the offset is in the ds segment but this maybe
	//	 * changed using any one of the segment override prefixes.
	//	 * Sample string:
	//	 *		dword ptr ds:[0040B000h]
	//	 *		word ptr ss:[0040B000h]
	//	 */
	//	 if( ! (pstState->insCurIns.wPrefixTypes & PREFIX_SEG) )	// ! higher precedence than &
	//	 {
	//		// No segment override prefix present, default to ds
	//		if(pstState->insCurIns.bImmType == IMM_16BIT)
	//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s ds:[%04Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	//		else
	//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s ds:[%08Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	//	 }
	//	 else
	//	 {
	//		// There is a segment override prefix. Print the correct one
	//		// by reading the stored prefix type in abPrefixes.
	//		switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
	//		{
	//			case OPC_PREFIX_SEGOVR_ES:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				else
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				break;

	//			case OPC_PREFIX_SEGOVR_CS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
	//				else
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
	//				break;

	//			case OPC_PREFIX_SEGOVR_SS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
	//				else
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
	//				break;

	//			case OPC_PREFIX_SEGOVR_DS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
	//				else
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
	//				break;
	//			
	//			case OPC_PREFIX_SEGOVR_FS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
	//				else
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
	//				break;
	//			
	//			case OPC_PREFIX_SEGOVR_GS:
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
	//				else
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
	//				break;

	//			default:	// Not a critical error
	//				nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"fStateDump(): Invalid segovr prefix %xh\n", pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG]);
	//				if(pstState->insCurIns.bImmType == IMM_16BIT)
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s <>:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				else
	//					nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s <>:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
	//								awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
	//				break;
	//		}// switch(abPrefixes)
	//	 
	//	 }// if PREFIX_SEG
	//}// if fDataOffset

	// 7: Go to next line
	nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"\r\n");

	// 8: Check if there are more bytes of the current instruction
	// to print.
	if(pstState->nBytesCurIns > 6)
	{
		// In the earlier print loop, nBytesPrinted is incremented after 
		// printing and before checking termination condition, so 
		// its value maybe greater than how many bytes were actually printed
		nBytesPrinted = 6;

		// Padding for address field
		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"            ");	// 12 spaces

		// Start from where we left off earlier
		register int nNewPrinted = 0;
		for(PBYTE pBytes = pstState->pbCurrentCodePtr - pstState->nBytesCurIns + nBytesPrinted;
		pBytes < pstState->pbCurrentCodePtr && nNewPrinted < pstState->nBytesCurIns;
		++pBytes, ++nNewPrinted)
		{
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%02X ", *pBytes);
		}
		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"\r\n");
	}

	pstState->dsNextState = DASM_STATE_RESET;
	return TRUE;

}

void DUMP_vDumpDataOffset(PDASMSTATE pstState)
{
	//if(pstState->insCurIns.fDesStrSet)
	//	wprintf_s(L",");

	/*
	 * By default, the offset is in the ds segment but this maybe
	 * changed using any one of the segment override prefixes.
	 * Sample string:
	 *		dword ptr ds:[0040B000h]
	 *		word ptr ss:[0040B000h]
	 */
	if( ! (pstState->insCurIns.wPrefixTypes & PREFIX_SEG) )	// ! higher precedence than &
	{
	// No segment override prefix present, default to ds
	if(pstState->insCurIns.bImmType == IMM_16BIT)
		wprintf_s(L"%sds:[%04Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	else
		wprintf_s(L"%sds:[%08Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	}
	else
	{
		// There is a segment override prefix. Print the correct one
		// by reading the stored prefix type in abPrefixes.

		WCHAR *pwszSegStr = NULL;

		DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
		if(pstState->insCurIns.bImmType == IMM_16BIT)
			wprintf_s(L"%s%s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
						pwszSegStr, pstState->insCurIns.iImm);
		else
			wprintf_s(L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
						pwszSegStr, pstState->insCurIns.iImm);
		
		//switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
		//{
		//	case OPC_PREFIX_SEGOVR_ES:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		else
		//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		break;

		//	case OPC_PREFIX_SEGOVR_CS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
		//		else
		//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
		//		break;

		//	case OPC_PREFIX_SEGOVR_SS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
		//		else
		//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
		//		break;

		//	case OPC_PREFIX_SEGOVR_DS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
		//		else
		//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
		//		break;
		//		
		//	case OPC_PREFIX_SEGOVR_FS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
		//		else
		//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
		//		break;
		//		
		//	case OPC_PREFIX_SEGOVR_GS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			wprintf_s(L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
		//		else
		//			wprintf_s(L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
		//		break;

		//	default:	// Not a critical error
		//		wprintf_s(L"fStateDump(): Invalid segovr prefix %xh\n", pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG]);
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			wprintf_s(L"%s <>:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		else
		//			wprintf_s(L"%s <>:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		break;
		//}// switch(abPrefixes)
		 
	}// if PREFIX_SEG
	return;
}

void DUMP_vDumpDataOffset_ToString(PDASMSTATE pstState, PDWORD pnTotalCharsWritten)
{
    ASSERT(pnTotalCharsWritten && *pnTotalCharsWritten > 0);

    PWCHAR pszStrStart;
    DWORD nTotalCharsCapacity;
    DWORD nTotalCharsWritten = *pnTotalCharsWritten;

    // Init
    pszStrStart = pstState->szDisassembledInst;
    nTotalCharsCapacity = _countof(pstState->szDisassembledInst);

	/*
	 * By default, the offset is in the ds segment but this maybe
	 * changed using any one of the segment override prefixes.
	 * Sample string:
	 *		dword ptr ds:[0040B000h]
	 *		word ptr ss:[0040B000h]
	 */
	if( ! (pstState->insCurIns.wPrefixTypes & PREFIX_SEG) )	// ! higher precedence than &
	{
	    // No segment override prefix present, default to ds
	    if(pstState->insCurIns.bImmType == IMM_16BIT)
		    nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%sds:[%04Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	    else
		    nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%sds:[%08Xh]", pstState->insCurIns.pwszPtrStr, pstState->insCurIns.iImm);
	}
	else
	{
		// There is a segment override prefix. Print the correct one
		// by reading the stored prefix type in abPrefixes.

		WCHAR *pwszSegStr = NULL;

		DUMP_vGetSegPrefix(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG], &pwszSegStr);
		if(pstState->insCurIns.bImmType == IMM_16BIT)
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
						pwszSegStr, pstState->insCurIns.iImm);
		else
			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s%s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
						pwszSegStr, pstState->insCurIns.iImm);
		
		//switch(pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG])
		//{
		//	case OPC_PREFIX_SEGOVR_ES:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		else
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		break;

		//	case OPC_PREFIX_SEGOVR_CS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
		//		else
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_CS], pstState->insCurIns.iImm);
		//		break;

		//	case OPC_PREFIX_SEGOVR_SS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
		//		else
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_SS], pstState->insCurIns.iImm);
		//		break;

		//	case OPC_PREFIX_SEGOVR_DS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
		//		else
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_DS], pstState->insCurIns.iImm);
		//		break;
		//		
		//	case OPC_PREFIX_SEGOVR_FS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
		//		else
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_FS], pstState->insCurIns.iImm);
		//		break;
		//		
		//	case OPC_PREFIX_SEGOVR_GS:
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
		//		else
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s %s:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_GS], pstState->insCurIns.iImm);
		//		break;

		//	default:	// Not a critical error
		//		nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"fStateDump(): Invalid segovr prefix %xh\n", pstState->insCurIns.abPrefixes[PREFIX_INDEX_SEG]);
		//		if(pstState->insCurIns.bImmType == IMM_16BIT)
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s <>:[%04Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		else
		//			nTotalCharsWritten += swprintf_s(pszStrStart + nTotalCharsWritten, nTotalCharsCapacity - nTotalCharsWritten, L"%s <>:[%08Xh]", pstState->insCurIns.pwszPtrStr, 
		//						awszSRegCodes[SREGCODE_ES], pstState->insCurIns.iImm);
		//		break;
		//}// switch(abPrefixes)
		 
	}// if PREFIX_SEG

    *pnTotalCharsWritten = nTotalCharsWritten;

	return;
}

// DUMP_vGetSegPrefix()
// Returns the segment register string given the segment override 
// prefix value as parameter. If there is no segment override 
// prefix/invalid is sent as the parameter, an empty string is returned.
void DUMP_vGetSegPrefix(BYTE bSegPrefixVal, __out WCHAR **ppwszPrefixStr)
{
	static WCHAR awszSegRegs[][4] = { L"", L"es:", L"cs:", L"ss:", L"ds:", L"fs:", L"gs:" };

	switch(bSegPrefixVal)
	{
		case OPC_PREFIX_SEGOVR_ES: *ppwszPrefixStr = awszSegRegs[1]; break;
		case OPC_PREFIX_SEGOVR_CS: *ppwszPrefixStr = awszSegRegs[2]; break;
		case OPC_PREFIX_SEGOVR_SS: *ppwszPrefixStr = awszSegRegs[3]; break;
		case OPC_PREFIX_SEGOVR_DS: *ppwszPrefixStr = awszSegRegs[4]; break;
		case OPC_PREFIX_SEGOVR_FS: *ppwszPrefixStr = awszSegRegs[5]; break;
		case OPC_PREFIX_SEGOVR_GS: *ppwszPrefixStr = awszSegRegs[6]; break;
		default: *ppwszPrefixStr = awszSegRegs[0]; break;
	}// switch(abPrefixes[])
	return;
}

/*
 * *************************
 *	Begin Opcode handlers
 * *************************
 */

/* 
 *	** PUSH **
 * FF /6	PUSH r/m16	Push r/m16
 * FF /6	PUSH r/m32	Push r/m32
 * 50+rw	PUSH r16	Push r16
 * 50+rd	PUSH r32	Push r32
 * 6A		PUSH imm8	Push imm8
 * 68		PUSH imm16	Push imm16
 * 68		PUSH imm32	Push imm32
 * 0E		PUSH CS		Push CS
 * 16		PUSH SS		Push SS
 * 1E		PUSH DS		Push DS
 * 06		PUSH ES		Push ES
 * 0F A0	PUSH FS		Push FS	// 2byte opcode
 * 0F A8	PUSH GS		Push GS	// 2byte opcode
 */
BOOL OPCHndlrStack_PUSH(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// pstState->pbCurrentCodePtr is now pointing to the byte following the opcode

	if( ! (bOpcode >= 0x50 && bOpcode <= 0x57) )
	{
		switch(bOpcode)
		{
			case 0xff:	// opsize attrib can be used and w-bit=1 always
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
				break;
			}

			case 0x6a:	// PUSH imm8
			{
				// size of immediate value is 8bits
				pstState->insCurIns.fImm = TRUE;
				pstState->insCurIns.bImmType = IMM_8BIT;
				pstState->dsNextState = DASM_STATE_IMM;
				break;
			}

			case 0x68:	// PUSH imm16 / PUSH imm32
			{
				// Size of immediate value is 16/32bits depending on the
				// presence of the operand size override prefix
				pstState->insCurIns.fImm = TRUE;
				if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
					pstState->insCurIns.bImmType = IMM_16BIT;
				else
					pstState->insCurIns.bImmType = IMM_32BIT;

				pstState->dsNextState = DASM_STATE_IMM;
				break;
			}

			case 0x0e:	// PUSH CS
			{	
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"cs");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			case 0x16:	// PUSH SS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"ss");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			case 0x1e:	// PUSH DS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"ds");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			case 0x06:	// PUSH ES
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"es");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			case 0xa0:	// 2byte opcode. 0fh has already been read. PUSH FS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"fs");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			case 0xa8:	// 2byte opcode. 0fh has already been read. PUSH GS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"gs");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			// No default statement required.
			// If opcode is not recognized at all, then
			// fRecognized will be FALSE and this function
			// will return FALSE.
			default:
			{
				wprintf_s(L"OPCHndlrStack_PUSH(): Invalid opcode %xh\n", bOpcode);
				return FALSE;
			}

		}// switch(bOpcode)
	}
	else
	{
		// Opcode is between 50h to 57h: PUSH reg
		// Get the register name
		WORD wReg = bOpcode - 0x50;
		
		if( pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE )
			// 16bit reg
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), 
								awszRegCodes16[wReg]);
		else
			// 32bit reg
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), 
								awszRegCodes32[wReg]);

		pstState->insCurIns.fSrcStrSet = TRUE;

		// Next state is DUMP because there are no more 
		// bytes to process for the current instruction.
		
		pstState->dsNextState = DASM_STATE_DUMP;
	
	}// if-else

	return TRUE;
}


/*	** POP **
 * 8F /0	POP m16
 * 8F /0	POP m32
 * 58+ rw	POP r16
 * 58+ rd	POP r32
 * 1F		POP DS
 * 07		POP ES
 * 17		POP SS
 * 0F A1	POP FS	// 2byte opcodes
 * 0F A9	POP GS
 */
BOOL OPCHndlrStack_POP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// POP opcodes
	if( ! (bOpcode >= 0x58 && bOpcode <= 0x5f) )
	{
		switch(bOpcode)
		{
			case 0x8f: // POP m32 / POP m16	// opsize attrib can be used, w=1 always
			{
				// there is a ModRM byte
				pstState->insCurIns.fModRM = TRUE;
				pstState->dsNextState = DASM_STATE_MOD_RM;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				break;
			}

 			case 0x1f: // POP DS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"ds");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

 			case 0x07: // POP ES
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"es");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

 			case 0x17: // POP SS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"ss");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

 			case 0xa1: // 2byte opcodes: 0fh has already been read. POP FS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"fs");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			case 0xa9: // 2byte opcodes: 0fh has already been read. POP GS
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"gs");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
				break;
			}

			default:
			{
				wprintf_s(L"OPCHndlrStack_POP(): Invalid opcode %xh\n", bOpcode);
				return FALSE;
			}

		}// switch(bOpcode)
	}
	else
	{
		// Opcode is between 58h to 5fh: POP reg

		// Get the register name index
		WORD wReg = bOpcode - 0x58;
		
		if( pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE )
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), awszRegCodes16[wReg]);
		else
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), awszRegCodes32[wReg]);

		pstState->insCurIns.fSrcStrSet = TRUE;

		// Next state is DUMP because there are no more
		// bytes to process for the current instruction.
		pstState->dsNextState = DASM_STATE_DUMP;

	}// if ! bOpcode >= 0x58 && bOpcode <= 0x5f

	return TRUE;
}


/*	** PUSHxx **
 * 60		PUSHA/PUSHAD
 * 9c		PUSHF/PUSHFD
 */
BOOL OPCHndlrStack_PUSHxx(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// If operand size prefix is present,
	// then 16bit mnemonic is used.
	// Remember that 32bit mnemonic is already written
	// in fStateOpcode()
	if(bOpcode == 0x60)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"pusha");
	}
	else if(bOpcode == 0x9c)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"pushf");
	}
	else
	{
		wprintf_s(L"OPCHndlrStack_PUSHxx(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** POPxx **
 * 61	POPA/POPAD
 * 9D	POPF/POPFD
 */
BOOL OPCHndlrStack_POPxx(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// If operand size prefix is present,
	// then 16bit mnemonic is used.
	// Remember that 32bit mnemonic is already written
	// in fStateOpcode()
	if(bOpcode == 0x61)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"popa");
	}
	else if(bOpcode == 0x9d)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"popf");
	}
	else
	{
		wprintf_s(L"OPCHndlrStack_POPxx(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*
 *		** ENTER **
 * C8 iw 00		ENTER imm16,0
 * C8 iw 01		ENTER imm16,1
 * C8 iw ib		ENTER imm16,imm8
 */
BOOL OPCHndlrStack_ENTER(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Because there are two immediate values in this instruction
	// we will handle it here itself and not in IMM state

	// pstState->pbCurrentCodePtr is pointing to the byte following opcode,
	// that is the first imm16 value

	WORD wImm16;
	BYTE bImmSecond;

	wImm16 = *((WORD*)pstState->pbCurrentCodePtr);
	pstState->pbCurrentCodePtr += 2;

	bImmSecond = *pstState->pbCurrentCodePtr;
	++(pstState->pbCurrentCodePtr);

	pstState->nBytesCurIns += sizeof(WORD) + sizeof(BYTE);	// imm16 and 0/1/imm8

	// imm16 into des str and imm8 into src str
	StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), L"%04Xh", wImm16);
	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), L"%02Xh", bImmSecond);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.fDesStrSet = TRUE;

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*
 * C9 LEAVE
 * C9 LEAVE
 */
BOOL OPCHndlrStack_LEAVE(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// Arithmetic instruction handlers

/*		** ADD/ADC **
 *
 * 04 ib		ADD AL,imm8
 * 05 iw		ADD AX,imm16
 * 05 id		ADD EAX,imm32
 * 80 /0 ib		ADD r/m8,imm8
 * 81 /0 iw		ADD r/m16,imm16
 * 81 /0 id		ADD r/m32,imm32
 * 83 /0 ib		ADD r/m16,imm8		with sign-extension
 * 83 /0 ib		ADD r/m32,imm8		with sign-extension
 * 00 /r		ADD r/m8,r8
 * 01 /r		ADD r/m16,r16
 * 01 /r		ADD r/m32,r32
 * 02 /r		ADD r8,r/m8
 * 03 /r		ADD r16,r/m16
 * 03 /r		ADD r32,r/m32
 *
 * 14 ib		ADC AL,imm8
 * 15 iw		ADC AX,imm16
 * 15 id		ADC EAX,imm32
 * 80 /2 ib		ADC r/m8,imm8
 * 81 /2 iw		ADC r/m16,imm16
 * 81 /2 id		ADC r/m32,imm32
 * 83 /2 ib		ADC r/m16,imm8 Add with CF sign-extended imm8 to r/m16
 * 83 /2 ib		ADC r/m32,imm8 Add with CF sign-extended imm8 into r/m32
 * 10 /r		ADC r/m8,r8
 * 11 /r		ADC r/m16,r16
 * 11 /r		ADC r/m32,r32
 * 12 /r		ADC r8,r/m8
 * 13 /r		ADC r16,r/m16
 * 13 /r		ADC r32,r/m32
 */
BOOL OPCHndlrALU_ADD(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	/*
	 * 80 - 83 is part of Multi instruction opcodes and is first handled
	 * in OPCHndlrMulti_8x() and then this function is called.
	 * So we can be sure that bOpcode is an ADD instruction only and
	 * we don't have to check the reg field of the ModRM byte.
	 */

	switch(bOpcode)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x04:
		case 0x14:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;

			// Construct the output string
			StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;

			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0x05:
		case 0x15:
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.bImmType = IMM_16BIT;
				// Construct the output string
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes16[REGCODE_AX]);
				pstState->insCurIns.fDesStrSet = TRUE;
			}
			else
			{
				pstState->insCurIns.bImmType = IMM_32BIT;
				// Construct the output string
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes32[REGCODE_EAX]);
				pstState->insCurIns.fDesStrSet = TRUE;
			}
			pstState->insCurIns.fImm = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0x80:
		case 0x81:
			// wbit and opsize attrib used to set bImmType in MODRM
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x83:	// with sign-extension
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.fImmSignEx = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		default:
			wprintf_s(L"OPCHndlrALU_ADD(): Invalid opcode: %x\n", bOpcode);
			return FALSE;

	}// switch(bOpcode)
	return TRUE;
}

/*		** SUB/SBB **
 * 1C ib	SBB AL,imm8
 * 1D iw	SBB AX,imm16
 * 1D id	SBB EAX,imm32
 *
 * 80 /3 ib		SBB r/m8,imm8
 * 81 /3 iw		SBB r/m16,imm16
 * 81 /3 id		SBB r/m32,imm32
 * 83 /3 ib		SBB r/m16,imm8		Subtract with borrow sign-extended imm8 from r/m16
 * 83 /3 ib		SBB r/m32,imm8		Subtract with borrow sign-extended imm8 from r/m32
 *
 * 18 /r	SBB r/m8,r8
 * 19 /r	SBB r/m16,r16
 * 19 /r	SBB r/m32,r32
 * 1A /r	SBB r8,r/m8
 * 1B /r	SBB r16,r/m16
 * 1B /r	SBB r32,r/m32
 * 
 * 2C ib SUB	AL,imm8
 * 2D iw SUB	AX,imm16
 * 2D id SUB	EAX,imm32
 *
 * 80 /5 ib		SUB r/m8,imm8
 * 81 /5 iw		SUB r/m16,imm16
 * 81 /5 id		SUB r/m32,imm32
 * 83 /5 ib		SUB r/m16,imm8		Subtract sign-extended imm8 from r/m16
 * 83 /5 ib		SUB r/m32,imm8		Subtract sign-extended imm8 from r/m32
 *
 * 28 /r	SUB r/m8,r8
 * 29 /r	SUB r/m16,r16
 * 29 /r	SUB r/m32,r32
 * 2A /r	SUB r8,r/m8
 * 2B /r	SUB r16,r/m16
 * 2B /r	SUB r32,r/m32
 *
 */
BOOL OPCHndlrALU_SUB(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	switch(bOpcode)
	{
		case 0x1c:
		case 0x2c:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0x1d:
		case 0x2d:
		{
			pstState->insCurIns.fImm = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				// 16bit operand
				pstState->insCurIns.bImmType = IMM_16BIT;
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), 
								awszRegCodes16[REGCODE_AX]);
				pstState->insCurIns.fDesStrSet = TRUE;
			}
			else
			{
				// 32bit
				pstState->insCurIns.bImmType = IMM_32BIT;
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), 
								awszRegCodes32[REGCODE_EAX]);
				pstState->insCurIns.fDesStrSet = TRUE;
			}

			pstState->dsNextState = DASM_STATE_IMM;
			break;
		}

		case 0x80:	// wbit and opsize attrib can be used to determine
		case 0x81:	// size of operands
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x83:	// imm8 always src with sign-extension
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fImmSignEx = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x18:	// wbit and opsize attrib can be used
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

	}// switch(bOpcode)
	return TRUE;
}

/*	** MUL/IMUL **
 * F6 /5		IMUL r/m8
 * F7 /5		IMUL r/m16
 * F7 /5		IMUL r/m32
 * 0F AF /r		IMUL r16,r/m16
 * 0F AF /r		IMUL r32,r/m32
 * 6B /r ib		IMUL r16,r/m16,imm8		word register = r/m16 * sign-extended immediate byte
 * 6B /r ib		IMUL r32,r/m32,imm8		doubleword register = r/m32 * sign-extended immediate byte
 * 6B /r ib		IMUL r16,imm8			word register =  word register * sign-extended immediate byte
 * 6B /r ib		IMUL r32,imm8			doubleword register =  doubleword register * signextended immediate byte
 * 69 /r iw		IMUL r16,r/m16,imm16	word register = r/m16 * immediate word
 * 69 /r id		IMUL r32,r/m32,imm32	doubleword register = r/m32 * immediate doubleword
 * 69 /r iw		IMUL r16,imm16			word register = r/m16 * immediate word
 * 69 /r id		IMUL r32,imm32			doubleword register = r/m32 * immediate doubleword
 *
 * F6 /4	MUL r/m8	Unsigned multiply (AX = AL * r/m8)
 * F7 /4	MUL r/m16	Unsigned multiply (DX:AX = AX * r/m16)
 * F7 /4	MUL r/m32	Unsigned multiply (EDX:EAX = EAX * r/m32)
 */
BOOL OPCHndlrALU_MUL(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Check when adding MUL instructions
	// Checked: difference is in the reg field of ModRM byte
	// and appropriate mnemonic will have been set by the 
	// OPCHndlrMulti_fx() function.
	switch(bOpcode)
	{
		case 0xf6:	// wBit and opsize attrib can be used
		case 0xf7:
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			break;

		case 0xaf:	// 2byte opcode
			// opsize attrib can be used
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			break;

		case 0x6b:
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fImmSignEx = TRUE;
			break;

		case 0x69:
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->insCurIns.fImm = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
				pstState->insCurIns.bImmType = IMM_16BIT;
			else
				pstState->insCurIns.bImmType = IMM_32BIT;
			break;

		default:
			wprintf_s(L"OPCHndlrALU_MUL(): Invalid opcode %xh\n", bOpcode);
			return FALSE;

	}// switch(bOpcode)

	// all variations have a ModRM byte
	pstState->insCurIns.fModRM = TRUE;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	
	return TRUE;
}

/*	** DIV **
 * F6 /6	DIV r/m8
 * F7 /6	DIV r/m16
 * F7 /6	DIV r/m32
 *
 * F6 /7	IDIV r/m8
 * F7 /7	IDIV r/m16
 * F7 /7	IDIV r/m32
 */
BOOL OPCHndlrALU_DIV(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// The reg/opcode field in the ModRM byte is different for
	// div and idiv, so the OPCHndlrMulti_fx() function would
	// have printed the correct mnemonic into pstState->wszCurInsStr.
	switch(bOpcode)
	{
		case 0xf6:	// wBit and opsize attrib can be used
		case 0xf7:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			break;

		default:
			wprintf_s(L"OPCHndlrALU_DIV(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
		
	}// switch(bOpcode)

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** INC **
 * FE /0	INC r/m8
 * FF /0	INC r/m16
 * FF /0	INC r/m32
 * 40+ rw	INC r16
 * 40+ rd	INC r32
 */
BOOL OPCHndlrALU_INC(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WORD wReg = bOpcode - OPC_INC_BEG;

	if(bOpcode >= OPC_INC_BEG && bOpcode <= OPC_INC_END)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), awszRegCodes16[wReg]);
		else
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), awszRegCodes32[wReg]);
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else if(bOpcode == 0xfe || bOpcode == 0xff)
	{
		// wBit and opsize attrib can be used
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
		pstState->dsNextState = DASM_STATE_MOD_RM;
	}
	else
	{
		wprintf_s(L"OPCHndlrALU_INC(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	return TRUE;
}

/*	** DEC **
 * FE /1	DEC r/m8
 * FF /1	DEC r/m16
 * FF /1	DEC r/m32
 * 48+rw	DEC r16
 * 48+rd	DEC r32
 */
BOOL OPCHndlrALU_DEC(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WORD wReg = bOpcode - OPC_DEC_BEG;

	if(bOpcode >= OPC_DEC_BEG && bOpcode <= OPC_DEC_END)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), awszRegCodes16[wReg]);
		else
			StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), awszRegCodes32[wReg]);
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else if(bOpcode == 0xfe || bOpcode == 0xff)
	{
		// wBit and opsize attrib can be used
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
		pstState->dsNextState = DASM_STATE_MOD_RM;
	}
	else
	{
		wprintf_s(L"OPCHndlrALU_DEC(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	return TRUE;
}

/*	** Rotate / Shift **
 *	* This is gonna be a big one! *
 *
 * C0 /0 ib		ROL r/m8,imm8
 * C0 /2 ib		RCL r/m8,imm8
 * C0 /3 ib		RCR r/m8,imm8
 * C1 /2 ib		RCL r/m16,imm8
 * C1 /2 ib		RCL r/m32,imm8
 * C1 /0 ib		ROL r/m16,imm8
 * C1 /0 ib		ROL r/m32,imm8
 * C0 /1 ib		ROR r/m8,imm8
 * C1 /1 ib		ROR r/m16,imm8
 * C1 /1 ib		ROR r/m32,imm8
 * C1 /3 ib		RCR r/m16,imm8
 * C1 /3 ib		RCR r/m32,imm8
 *
 * D0 /2	RCL r/m8,1
 * D2 /2	RCL r/m8,CL
 * D1 /2	RCL r/m16,1
 * D3 /2	RCL r/m16,CL
 * D1 /2	RCL r/m32,1
 * D3 /2	RCL r/m32,CL
 *
 * D0 /3	RCR r/m8,1
 * D2 /3	RCR r/m8,CL
 * D1 /3	RCR r/m16,1
 * D3 /3	RCR r/m16,CL
 * D1 /3	RCR r/m32,1
 * D3 /3	RCR r/m32,CL
 *
 * D0 /0	ROL r/m8,1
 * D2 /0	ROL r/m8,CL
 * D1 /0	ROL r/m16,1
 * D3 /0	ROL r/m16,CL
 * D1 /0	ROL r/m32,1
 * D3 /0	ROL r/m32,CL
 *
 * D0 /1	ROR r/m8,1
 * D2 /1	ROR r/m8,CL
 * D1 /1	ROR r/m16,1
 * D3 /1	ROR r/m16,CL
 * D1 /1	ROR r/m32,1
 * D3 /1	ROR r/m32,CL
 *
 * 0F A4 SHLD r/m16,r16,imm8	Shift r/m16 to left imm8 places while shifting bits from r16 in from the right
 * 0F A5 SHLD r/m16,r16,CL		Shift r/m16 to left CL places while shifting bits from r16 in from the right
 * 0F A4 SHLD r/m32,r32,imm8	Shift r/m32 to left imm8 places while shifting bits from r32 in from the right
 * 0F A5 SHLD r/m32,r32,CL		Shift r/m32 to left CL places while shifting bits from r32 in from the right
 *
 * 0F AC SHRD r/m16,r16,imm8	Shift r/m16 to right imm8 places while shifting bits from r16 in from the left
 * 0F AD SHRD r/m16,r16,CL		Shift r/m16 to right CL places while shifting bits from r16 in from the left
 * 0F AC SHRD r/m32,r32,imm8	Shift r/m32 to right imm8 places while shifting bits from r32 in from the left
 * 0F AD SHRD r/m32,r32,CL		Shift r/m32 to right CL places while shifting bits from r32 in from the left
 *
 */
BOOL OPCHndlrALU_Shift(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(pstState->bOpcodeHigh != 0xA)	// not shrd or shld
		if( ! OPCHndlrALU_Shift_SetInsStr(pstState, bOpcode) )
			return FALSE;

	switch(bOpcode)
	{
		case 0xc0:
		case 0xc1:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		// Destination is either a reg/mem and it can be 
		// 8/16/32bits. This can be determined by w-bit,
		// Mod/RM combinations in the MODRM state.
		// Only difference is the source operand: CL/1
		case 0xd0:	// src = 1
		case 0xd1:	
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			StringCchPrintf(pstState->insCurIns.wszCurInsStrOpr3, 
							_countof(pstState->insCurIns.wszCurInsStrOpr3), L"1");
			pstState->insCurIns.fOpr3StrSet = TRUE;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0xd2:	// src = CL register
		case 0xd3:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			StringCchPrintf(pstState->insCurIns.wszCurInsStrOpr3, 
							_countof(pstState->insCurIns.wszCurInsStrOpr3), L"cl");
			pstState->insCurIns.fOpr3StrSet = TRUE;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0xa4:	// SHLD r/m16,r16,imm8
		case 0xac:	// SHRD r/m16,r16,imm8
			pstState->insCurIns.fSpecialInstruction = TRUE;		// No w-bit
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
				pstState->insCurIns.bOperandSizeDes = pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
			else
				pstState->insCurIns.bOperandSizeDes = pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0xa5:	// shld ax,bx,cl
		case 0xad:	// shrd ax,bx,cl
			pstState->insCurIns.fSpecialInstruction = TRUE;		// No w-bit
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
				pstState->insCurIns.bOperandSizeDes = pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
			else
				pstState->insCurIns.bOperandSizeDes = pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;

			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->insCurIns.fOpr3StrSet = TRUE;
			StringCchPrintf(pstState->insCurIns.wszCurInsStrOpr3, 
							_countof(pstState->insCurIns.wszCurInsStrOpr3), L"cl");
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		
		default:
			wprintf_s(L"OPCHndlrALU_Shift(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
		
	}// switch(bOpcode)
	return TRUE;
}

// OpcodeEx		Ins
//		0		ROL
//		1		ROR
//		2		RCL
//		3		RCR
//		4		SHL/SAL
//		5		SHR
//		6		SAL
//		7		SAR
BOOL OPCHndlrALU_Shift_SetInsStr(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh %xh\n", __FUNCTIONW__, bOpcode, *pstState->pbCurrentCodePtr);
	#endif

	// Check the 'reg/opcode' field of the ModRM byte
	// This is the extension to the primary opcode.
	BYTE bOpcodeEx;

	static WCHAR awszMnemonics[][MAX_OPCODE_NAME_LEN+1] = {
					L"rol",	L"ror",	L"rcl",	L"rcr", L"shl", L"shr", L"sal", L"sar"};

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);
	if(bOpcodeEx < 0 || bOpcodeEx > 7)
		return FALSE;

	StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[bOpcodeEx]);
	
	return TRUE;
}

BOOL OPCHndlrALU_SALC(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

// Logical instructions

/*	** OR **
 * 0C ib	OR AL,imm8
 * 0D iw	OR AX,imm16
 * 0D id	OR EAX,imm32
 *
 * 80 /1 ib		OR r/m8,imm8
 * 81 /1 iw		OR r/m16,imm16
 * 81 /1 id		OR r/m32,imm32
 * 83 /1 ib		OR r/m16,imm8		r/m16 OR imm8 (sign-extended)
 * 83 /1 ib		OR r/m32,imm8		r/m32 OR imm8 (sign-extended)
 *
 * 08 /r	OR r/m8,r8
 * 09 /r	OR r/m16,r16
 * 09 /r	OR r/m32,r32
 * 0A /r	OR r8,r/m8
 * 0B /r	OR r16,r/m16
 * 0B /r	OR r32,r/m32
 */
BOOL OPCHndlrALU_OR(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	switch(bOpcode)
	{
		case 0x0c:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0x0d:
		{
			pstState->insCurIns.fImm = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.bImmType = IMM_16BIT;
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes16[REGCODE_AX]);
			}
			else
			{
				pstState->insCurIns.bImmType = IMM_32BIT;
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes32[REGCODE_EAX]);
			}
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		
		}// case 0x0d

		case 0x80:	// bImmType can be set in the MODRM state using wBit and opsize attrib
		case 0x81:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x83:	// 83 /1 ib		OR r/m16,imm8
					// 83 /1 ib		OR r/m32,imm8
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fImmSignEx = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x08:	// 08 /r	OR r/m8,r8
		case 0x09:
		case 0x0a:
		case 0x0b:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		default:
			wprintf_s(L"OPCHndlrALU_OR(): Invalid opcode: %xh\n", bOpcode);
			return FALSE;

	}// switch(bOpcode)
	return TRUE;
}

/*		** AND **
 * 24 ib		AND AL,imm8
 * 25 iw		AND AX,imm16
 * 25 id		AND EAX,imm32
 * 80 /4 ib 	AND r/m8,imm8
 * 81 /4 iw 	AND r/m16,imm16
 * 81 /4 id 	AND r/m32,imm32
 * 83 /4 ib 	AND r/m16,imm8	(sign-extended)
 * 83 /4 ib 	AND r/m32,imm8  (sign-extended)
 * 20 /r		AND r/m8,r8
 * 21 /r		AND r/m16,r16
 * 21 /r		AND r/m32,r32
 * 22 /r		AND r8,r/m8
 * 23 /r		AND r16,r/m16
 * 23 /r		AND r32,r/m32
 */
BOOL OPCHndlrALU_AND(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	switch(bOpcode)
	{

		case 0x20:	// /r indicates that there is a ModRM byte next
		case 0x21:
		case 0x22:
		case 0x23:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x24:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;

			// Construct the output string
			StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;

			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0x25:
			pstState->insCurIns.fImm = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.bImmType = IMM_16BIT;
				// Construct the output string
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes16[REGCODE_AX]);
			}
			else
			{
				pstState->insCurIns.bImmType = IMM_32BIT;
				// Construct the output string
				StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), awszRegCodes32[REGCODE_EAX]);
			}
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;

			break;

		case 0x80:	// wbit and opsize attrib
		case 0x81:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x83:
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.fImmSignEx = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		default:
			wprintf_s(L"OPCHndlrALU_AND(): Invalid opcode: %xh\n", bOpcode);
			return FALSE;

	}// switch(bOpcode)
	return TRUE;
}

/*	** XOR **
 * 34 ib	XOR AL,imm8
 * 35 iw	XOR AX,imm16
 * 35 id	XOR EAX,imm32
 *
 * 80 /6 ib		XOR r/m8,imm8
 * 81 /6 iw		XOR r/m16,imm16
 * 81 /6 id		XOR r/m32,imm32
 * 83 /6 ib		XOR r/m16,imm8		r/m16 XOR imm8 (sign-extended)
 * 83 /6 ib		XOR r/m32,imm8		r/m32 XOR imm8 (sign-extended)
 *
 * 30 /r	XOR r/m8,r8
 * 31 /r	XOR r/m16,r16
 * 31 /r	XOR r/m32,r32
 * 32 /r	XOR r8,r/m8
 * 33 /r	XOR r16,r/m16
 * 33 /r	XOR r32,r/m32
 */
BOOL OPCHndlrALU_XOR(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	switch(bOpcode)
	{
		case 0x34:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;

			// Construct the output string
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0x35:
		{
			pstState->insCurIns.fImm = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.bImmType = IMM_16BIT;
				// Construct the output string
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes16[REGCODE_AX]);
			}
			else
			{
				pstState->insCurIns.bImmType = IMM_32BIT;
				// Construct the output string
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes32[REGCODE_EAX]);
			}
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		
		}//case 0x35

		case 0x80:	// op size can be determined by wbit and op size prefix
		case 0x81:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->insCurIns.fImm = TRUE;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x83:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fImmSignEx = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x30:
		case 0x31:
		case 0x32:	// d-bit is set
		case 0x33:	
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		default:
			wprintf_s(L"OPCHndlrALU_XOR(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
	}

	return TRUE;
}

/*	** NOT **
 * F6 /2	NOT r/m8	Reverse each bit of r/m8
 * F7 /2	NOT r/m16	Reverse each bit of r/m16
 * F7 /2	NOT r/m32	Reverse each bit of r/m32
 */
BOOL OPCHndlrALU_NOT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** NEG **
 * F6 /3	NEG r/m8	Two’s complement negate r/m8
 * F7 /3	NEG r/m16	Two’s complement negate r/m16
 * F7 /3	NEG r/m32	Two’s complement negate r/m32
 */
BOOL OPCHndlrALU_NEG(PDASMSTATE pstState, BYTE bOpcode)
{	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


// Adjust instructions

/*	** DAA **
 * 27	DAA		Decimal adjust AL after addition
 */
BOOL OPCHndlrALU_DAA(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** DAS **
 * 2F	DAS		Decimal adjust AL after subtraction
 */
BOOL OPCHndlrALU_DAS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

BOOL OPCHndlrALU_AAA(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*
 * 3F	AAS
 */
BOOL OPCHndlrALU_AAS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*
 * D4 0A	AAM
 * D4 ib	AAM	 - not handled
 */
BOOL OPCHndlrALU_AAM(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// move past 0a after the opcode d4
	++(pstState->pbCurrentCodePtr);

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


/*
 * D5 0A	AAD
 * D5 ib	AAD	 - not handled
 */
BOOL OPCHndlrALU_AAD(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// move past 0a after the opcode d5
	++pstState->pbCurrentCodePtr;

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// Memory operations

/*	** XCHG **
 * 90+rw	XCHG AX,r16		Exchange r16 with AX
 * 90+rw	XCHG r16,AX		Exchange AX with r16
 * 90+rd	XCHG EAX,r32	Exchange r32 with EAX
 * 90+rd	XCHG r32,EAX	Exchange EAX with r32
 *
 * 86 /r XCHG r/m8,r8		Exchange r8 (byte register) with byte from r/m8
 * 86 /r XCHG r8,r/m8		Exchange byte from r/m8 with r8 (byte register)
 * 87 /r XCHG r/m16,r16		Exchange r16 with word from r/m16
 * 87 /r XCHG r16,r/m16		Exchange word from r/m16 with r16
 * 87 /r XCHG r/m32,r32		Exchange r32 with doubleword from r/m32
 * 87 /r XCHG r32,r/m32		Exchange doubleword from r/m32 with r32
 */
BOOL OPCHndlrMem_XCHG(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(bOpcode >= OPC_XCHG_BEG && bOpcode <= OPC_XCHG_END)
	{
		BYTE bReg = bOpcode - OPC_XCHG_BEG;

		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
		{
			// 16bit regs
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes16[REGCODE_AX]);
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								awszRegCodes16[bReg]);
		}
		else
		{
			// 32bit regs
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes32[REGCODE_AX]);
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								awszRegCodes32[bReg]);
		}
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->insCurIns.fDesStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else if(bOpcode == 0x86 || bOpcode == 0x87)
	{
		// op size can be determined by w-bit and op size attrib
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;
		pstState->dsNextState = DASM_STATE_MOD_RM;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_XCHG(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}
	return TRUE;
}

/*	** MOV **
 * 88 /r	MOV r/m8,r8			Move r8 to r/m8
 * 89 /r	MOV r/m16,r16		Move r16 to r/m16
 * 89 /r	MOV r/m32,r32		Move r32 to r/m32
 * 8A /r	MOV r8,r/m8			Move r/m8 to r8
 * 8B /r	MOV r16,r/m16		Move r/m16 to r16
 * 8B /r	MOV r32,r/m32		Move r/m32 to r32
 * 8C /r	MOV r/m16,Sreg**	Move segment register to r/m16
 * 8E /r	MOV Sreg,r/m16**	Move r/m16 to segment register
 *
 * A0	MOV AL,moffs8*		Move byte at (seg:offset) to AL
 * A1	MOV AX,moffs16*		Move word at (seg:offset) to AX
 * A1	MOV EAX,moffs32*	Move doubleword at (seg:offset) to EAX
 * A2	MOV moffs8*,AL		Move AL to (seg:offset)
 * A3	MOV moffs16*,AX		Move AX to (seg:offset)
 * A3	MOV moffs32*,EAX	Move EAX to (seg:offset)
 *
 * B0+ rb	MOV r8,imm8
 * B8+ rw	MOV r16,imm16
 * B8+ rd	MOV r32,imm32
 *
 * C6 /0	MOV r/m8,imm8
 * C7 /0	MOV r/m16,imm16
 * C7 /0	MOV r/m32,imm32
 *
 * * The moffs8, moffs16, and moffs32 operands specify a simple offset relative to the segment base, where
 * 8, 16, and 32 refer to the size of the data. The address-size attribute of the instruction determines 
 * the size of the offset, either 16 or 32 bits.
 *
 * ** In 32-bit mode, the assembler may insert the 16-bit operand-size prefix with this instruction.
 *
 */
BOOL OPCHndlrMem_MOV(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	switch(bOpcode)
	{

		case 0x88:	// wBit and opsize attrib used
		case 0x89:
		case 0x8a:
		case 0x8b:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x8c:	// 8C /r	MOV r/m16,Sreg
		{	
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;

			pstState->insCurIns.fSpecialInstruction = TRUE;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_SREG;
			pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;	// only if Mod+RM bits suggest a register destination
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		}

		case 0x8e:	// 8C /r	MOV Sreg,r/m16
		{	
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;

			pstState->insCurIns.fSpecialInstruction = TRUE;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_GREG;	// only if Mod+RM bits suggest a register source
			pstState->insCurIns.bRegTypeDest = REGTYPE_SREG;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		}

		case 0xa0:	// A0	MOV AL,moffs8*		Move byte at (seg:offset) to AL
		{
			pstState->insCurIns.fDataOffset = TRUE;

			// determine offset size using address size attrib
			if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
				// 16bit offset
				pstState->insCurIns.bImmType = IMM_16BIT;
			else
				// 32bit offset
				pstState->insCurIns.bImmType = IMM_32BIT;

			pstState->insCurIns.pwszPtrStr = awszPtrStr[PTR_STR_INDEX_BYTE];
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
							L"%s", awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		}

		case 0xa1:	// A1	MOV AX,moffs16*
		{
			pstState->insCurIns.fDataOffset = TRUE;

			// determine offset size using address size attrib
			if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
				// 16bit offset
				pstState->insCurIns.bImmType = IMM_16BIT;
			else
				// 32bit offset
				pstState->insCurIns.bImmType = IMM_32BIT;

			// determine operand size using opsize attrib
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.pwszPtrStr = awszPtrStr[PTR_STR_INDEX_WORD];
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"%s", awszRegCodes16[REGCODE_AX]);
			}
			else
			{
				pstState->insCurIns.pwszPtrStr = awszPtrStr[PTR_STR_INDEX_DWORD];
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
									L"%s", awszRegCodes32[REGCODE_EAX]);
			}

			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		}

		case 0xa2:	// A2	MOV moffs8*,AL
		{
			pstState->insCurIns.fDataOffset = TRUE;

			// determine offset size using address size attrib
			if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
				// 16bit offset
				pstState->insCurIns.bImmType = IMM_16BIT;
			else
				// 32bit offset
				pstState->insCurIns.bImmType = IMM_32BIT;

			pstState->insCurIns.pwszPtrStr = awszPtrStr[PTR_STR_INDEX_BYTE];
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
							L"%s", awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fSrcStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		}

		case 0xa3:	// A3	MOV moffs16*,AX
		{
			pstState->insCurIns.fDataOffset = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
				// 16bit offset
				pstState->insCurIns.bImmType = IMM_16BIT;
			else
				// 32bit offset
				pstState->insCurIns.bImmType = IMM_32BIT;
			
			// determine operand size using opsize attrib
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.pwszPtrStr = awszPtrStr[PTR_STR_INDEX_WORD];
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"%s", awszRegCodes16[REGCODE_AX]);
			}
			else
			{
				pstState->insCurIns.pwszPtrStr = awszPtrStr[PTR_STR_INDEX_DWORD];
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"%s", awszRegCodes32[REGCODE_EAX]);
			}
			
			pstState->insCurIns.fSrcStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		}

		case 0xc6:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0xc7:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->insCurIns.fImm = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
				pstState->insCurIns.bImmType = IMM_16BIT;
			else
				pstState->insCurIns.bImmType = IMM_32BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		default:
		{
			if(bOpcode >= 0xb0 && bOpcode <= 0xb7)
			{
				BYTE bReg = bOpcode - 0xb0;
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), 
								L"%s", awszRegCodes8[bReg]);
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->insCurIns.fImm = TRUE;
				pstState->insCurIns.bImmType = IMM_8BIT;
				pstState->dsNextState = DASM_STATE_IMM;
			}
			else if(bOpcode >= 0xb8 && bOpcode <= 0xbf)
			{
				BYTE bReg = bOpcode - 0xb8;
				if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
				{
					StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), 
									L"%s", awszRegCodes16[bReg]);
					pstState->insCurIns.fDesStrSet = TRUE;
					pstState->insCurIns.bImmType = IMM_16BIT;
				}
				else
				{
					StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), 
									L"%s", awszRegCodes32[bReg]);
					pstState->insCurIns.fDesStrSet = TRUE;
					pstState->insCurIns.bImmType = IMM_32BIT;
				}
				pstState->insCurIns.fImm = TRUE;
				pstState->dsNextState = DASM_STATE_IMM;
			}
			else
			{
				wprintf_s(L"OPCHndlrMem_MOV(): Invalid opcode %xh\n", bOpcode);
				return FALSE;
			}
			
		}// default


	}// switch(bOpcode)
	return TRUE;
}

/*	** LEA **
 * 8D /r	LEA r16,m	Store effective address for m in register r16
 * 8D /r	LEA r32,m	Store effective address for m in register r32
 */
BOOL OPCHndlrMem_LEA(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;

	/*
	 * Manually set bDBit because it is insignificant for this instruction
	 * but MODRM state depends on it having the correct value.
	 * If we continue with bDBit = 0, then this instruction will be 
	 * decoded as LEA m, r32 :P
	 */
	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_UNDEF;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	}
	else
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_UNDEF;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/* ** CWDE **
 * 98 CBW	AX = sign-extend of AL
 * 98 CWDE	EAX = sign-extend of AX
 */
BOOL OPCHndlrMem_CWDE(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Override the pstState->wszCurInsStr because mnemonic depends 
	// on the operand size
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cbw");
	}
	else
	{
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cwde");
	}
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** CWD/CDQ **
 * 99	CWD		DX:AX = sign-extend of AX
 * 99	CDQ		EDX:EAX = sign-extend of EAX
 */
BOOL OPCHndlrMem_CDQ(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Override the pstState->wszCurInsStr because mnemonic depends 
	// on the operand size
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cwd");
	}
	else
	{
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cdq");
	}
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** SAHF **
 * 9E	SAHF	Loads SF, ZF, AF, PF, and CF from AH into EFLAGS register
 */
BOOL OPCHndlrMem_SAHF(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

BOOL OPCHndlrMem_LAHF(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** MOVS **
 * A4	MOVSB	Move byte at address DS:(E)SI to address ES:(E)DI
 * A5	MOVSW	Move word at address DS:(E)SI to address ES:(E)DI
 * A5	MOVSD	Move doubleword at address DS:(E)SI to address ES:(E)DI
 */
BOOL OPCHndlrMem_MOVS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WCHAR *pwszxDI = NULL;
	WCHAR *pwszxSI = NULL;
	WCHAR *pwszPtrStr = NULL;
	
	// Determine DI/EDI based on address size attrib
	if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
	{
		pwszxDI = awszRegCodes16[REGCODE_DI];
		pwszxSI = awszRegCodes16[REGCODE_SI];
	}
	else
	{
		pwszxDI = awszRegCodes32[REGCODE_EDI];
		pwszxSI = awszRegCodes32[REGCODE_ESI];
	}

	// to print: movs    byte ptr es:[edi],byte ptr [esi]
	if(bOpcode == 0xA4)
		pwszPtrStr = awszPtrStr[PTR_STR_INDEX_BYTE];
	else if(bOpcode == 0xA5)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_WORD];
		else
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_DWORD];
	}

	// destination: <ptrStr> es:[(e)di]
	StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
						L"%ses:[%s]", pwszPtrStr, pwszxDI);

	// source: <ptrStr> [(e)si]
	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
					L"%s[%s]", pwszPtrStr, pwszxSI);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** LODS **
 * AC	LODS m8		Load byte at address DS:(E)SI into AL
 * AD	LODS m16	Load word at address DS:(E)SI into AX
 * AD	LODS m32	Load doubleword at address DS:(E)SI into EAX
 * AC	LODSB		Load byte at address DS:(E)SI into AL
 * AD	LODSW		Load word at address DS:(E)SI into AX
 * AD	LODSD		Load doubleword at address DS:(E)SI into EAX
 */
BOOL OPCHndlrMem_LODS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WCHAR *pwszxSI = NULL;
	WCHAR *pwszPtrStr = NULL;
	
	// Determine DI/EDI based on address size attrib
	if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
		pwszxSI = awszRegCodes16[REGCODE_SI];
	else
		pwszxSI = awszRegCodes32[REGCODE_ESI];

	if(bOpcode == 0xAC)
		pwszPtrStr = awszPtrStr[PTR_STR_INDEX_BYTE];
	else if(bOpcode == 0xAD)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_WORD];
		else
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_DWORD];
	}

	// to print: lods        dword ptr [esi]
	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
					L"%s[%s]", pwszPtrStr, pwszxSI);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** STOSx **
 * AA STOS m8	Store AL at address ES:(E)DI
 * AB STOS m16	Store AX at address ES:(E)DI
 * AB STOS m32	Store EAX at address ES:(E)DI
 * AA STOSB		Store AL at address ES:(E)DI
 * AB STOSW		Store AX at address ES:(E)DI
 * AB STOSD		Store EAX at address ES:(E)DI
 */
BOOL OPCHndlrMem_STOS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WCHAR *pwszxDI = NULL;
	WCHAR *pwszPtrStr = NULL;
	
	// Determine DI/EDI based on address size attrib
	if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
		pwszxDI = awszRegCodes16[REGCODE_DI];
	else
		pwszxDI = awszRegCodes32[REGCODE_EDI];

	if(bOpcode == 0xAA)
		pwszPtrStr = awszPtrStr[PTR_STR_INDEX_BYTE];
	else if(bOpcode == 0xAB)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_WORD];
		else
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_DWORD];
	}

	// to print: stos        word ptr es:[edi]
	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
					L"%ses:[%s]", pwszPtrStr, pwszxDI);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** LES **
 * C4 /r	LES r16,m16:16	Load ES:r16 with far pointer from memory
 * C4 /r	LES r32,m16:32	Load ES:r32 with far pointer from memory
 */
BOOL OPCHndlrMem_LES(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;

	pstState->insCurIns.fSpecialInstruction = TRUE;
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	}
	else
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** LDS **
 * C5 /r	LDS r16,m16:16	Load DS:r16 with far pointer from memory
 * C5 /r	LDS r32,m16:32	Load DS:r32 with far pointer from memory
 */
BOOL OPCHndlrMem_LDS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;

	pstState->insCurIns.fSpecialInstruction = TRUE;
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	}
	else
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** XLAT **
 * D7 XLAT m8	Set AL to memory byte DS:[(E)BX + unsigned AL]
 * D7 XLATB		Set AL to memory byte DS:[(E)BX + unsigned AL]
 */
BOOL OPCHndlrMem_XLAT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// XLAT m8 is only for documentation purposes in the code
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// Control flow and Conditional instructions

/*	** CMP **
 * 3C ib		CMP AL, imm8
 * 3D iw		CMP AX, imm16
 * 3D id		CMP EAX, imm32
 * 80 /7 ib		CMP r/m8, imm8
 * 81 /7 iw		CMP r/m16, imm16
 * 81 /7 id		CMP r/m32,imm32
 * 83 /7 ib		CMP r/m16,imm8
 * 83 /7 ib		CMP r/m32,imm8
 * 38 /r		CMP r/m8,r8
 * 39 /r		CMP r/m16,r16
 * 39 /r		CMP r/m32,r32
 * 3A /r		CMP r8,r/m8
 * 3B /r		CMP r16,r/m16
 * 3B /r		CMP r32,r/m32
 *
 * When an immediate value is used as an operand, it is signextended
 * to the length of the first operand.
 */
BOOL OPCHndlrCC_CMP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	switch(bOpcode)
	{

		case 0x3c:
		{
			pstState->insCurIns.bImmType = IMM_8BIT;
			// Construct the output string
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->insCurIns.fImm = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		}

		case 0x3d:
		{
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.bImmType = IMM_16BIT;
				// Construct the output string
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes16[REGCODE_AX]);
			}
			else
			{
				pstState->insCurIns.bImmType = IMM_32BIT;
				// Construct the output string
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes16[REGCODE_EAX]);
			}
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->insCurIns.fImm = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;
		}

		case 0x38:	// /r
		case 0x39:
		case 0x3a:
		case 0x3b:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x80:
		case 0x81:
		{
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		}

		case 0x83:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;
			pstState->insCurIns.fImmSignEx = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

	}// switch(bOpcode)

	return TRUE;
}

/*	** BOUND **
 * 62 /r	BOUND r16,m16&16	Check if r16 (array index) is within bounds specified by m16&16
 * 62 /r	BOUND r32,m32&32	Check if r32 (array index) is within bounds specified by m32&32
 */
BOOL OPCHndlrCC_BOUND(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;	// ptr str selector
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	}
	else
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;	// ptr str selector
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*	** ARPL **
 * 63 /r	ARPL r/m16,r16	Adjust RPL of r/m16 to not less than RPL of r16
 */
BOOL OPCHndlrCC_ARPL(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
	pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	// pstState->pbCurrentCodePtr is already pointing to ModRM byte

	return TRUE;
}


/*	** Jcc **
 * 77 cb JA		rel8 Jump short if above (CF=0 and ZF=0)
 * 73 cb JAE	rel8 Jump short if above or equal (CF=0)
 * 72 cb JB		rel8 Jump short if below (CF=1)
 * 76 cb JBE	rel8 Jump short if below or equal (CF=1 or ZF=1)
 * E3 cb JCXZ	rel8 Jump short if CX register is 0
 * E3 cb JECXZ	rel8 Jump short if ECX register is 0
 * 74 cb JE		rel8 Jump short if equal (ZF=1)
 * 7F cb JG		rel8 Jump short if greater (ZF=0 and SF=OF)
 * 7D cb JGE	rel8 Jump short if greater or equal (SF=OF)
 * 7C cb JL		rel8 Jump short if less (SF<>OF)
 * 7E cb JLE	rel8 Jump short if less or equal (ZF=1 or SF<>OF)
 * 75 cb JNE	rel8 Jump short if not equal (ZF=0)
 * 71 cb JNO	rel8 Jump short if not overflow (OF=0)
 * 7B cb JNP	rel8 Jump short if not parity (PF=0)
 * 79 cb JNS	rel8 Jump short if not sign (SF=0)
 * 70 cb JO		rel8 Jump short if overflow (OF=1)
 * 7A cb JP		rel8 Jump short if parity (PF=1)
 * 78 cb JS		rel8 Jump short if sign (SF=1)
 *
 * 0F 87 cw/cd JA rel16/32	Jump near if above (CF=0 and ZF=0)
 * 0F 83 cw/cd JAE rel16/32 Jump near if above or equal (CF=0)
 * 0F 82 cw/cd JB rel16/32	Jump near if below (CF=1)
 * 0F 86 cw/cd JBE rel16/32 Jump near if below or equal (CF=1 or ZF=1)
 * 0F 84 cw/cd JE rel16/32	Jump near if equal (ZF=1)
 * 0F 8F cw/cd JG rel16/32	Jump near if greater (ZF=0 and SF=OF)
 * 0F 8D cw/cd JGE rel16/32 Jump near if greater or equal (SF=OF)
 * 0F 8C cw/cd JL rel16/32	Jump near if less (SF<>OF)
 * 0F 8E cw/cd JLE rel16/32 Jump near if less or equal (ZF=1 or SF<>OF)
 * 0F 85 cw/cd JNE rel16/32 Jump near if not equal (ZF=0)
 * 0F 81 cw/cd JNO rel16/32 Jump near if not overflow (OF=0)
 * 0F 8B cw/cd JNP rel16/32 Jump near if not parity (PF=0)
 * 0F 89 cw/cd JNS rel16/32 Jump near if not sign (SF=0)
 * 0F 80 cw/cd JO rel16/32	Jump near if overflow (OF=1)
 * 0F 8A cw/cd JP rel16/32	Jump near if parity (PF=1)
 * 0F 88 cw/cd JS rel16/32	Jump near if sign (SF=1)
 *
 * EB cb JMP rel8		Jump short, relative, displacement relative to next instruction
 * E9 cw JMP rel16		Jump near, relative, displacement relative to next instruction
 * E9 cd JMP rel32		Jump near, relative, displacement relative to next instruction
 * FF /4 JMP r/m16		Jump near, absolute indirect, address given in r/m16
 * FF /4 JMP r/m32		Jump near, absolute indirect, address given in r/m32
 * EA cd JMP ptr16:16	Jump far, absolute, address given in operand
 * EA cp JMP ptr16:32	Jump far, absolute, address given in operand
 * FF /5 JMP m16:16		Jump far, absolute indirect, address given in m16:16
 * FF /5 JMP m16:32		Jump far, absolute indirect, address given in m16:32
 */
BOOL OPCHndlrCC_JUMP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(bOpcode >= OPC_JMP_BEG && bOpcode <= OPC_JMP_END)
	{
		pstState->insCurIns.fCodeOffset = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
		pstState->dsNextState = DASM_STATE_IMM;
	}
	else if(bOpcode >= 0x80 && bOpcode <= 0x8f)	// 2byte opcodes
	{
		pstState->insCurIns.fCodeOffset = TRUE;
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pstState->insCurIns.bImmType = IMM_16BIT;
		else
			pstState->insCurIns.bImmType = IMM_32BIT;
		pstState->dsNextState = DASM_STATE_IMM;
	}
	else
	{
		switch(bOpcode)
		{
			case 0xe3:	// jcxz/jcexz
			{
				pstState->insCurIns.fCodeOffset = TRUE;
				if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
					StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", L"jcxz");
				pstState->insCurIns.bImmType = IMM_8BIT;
				pstState->dsNextState = DASM_STATE_IMM;
				break;
			}

			case 0xeb:
				pstState->insCurIns.fCodeOffset = TRUE;
				pstState->insCurIns.bImmType = IMM_8BIT;
				pstState->dsNextState = DASM_STATE_IMM;
				break;
			
			case 0xe9:
				pstState->insCurIns.fCodeOffset = TRUE;
				if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
					pstState->insCurIns.bImmType = IMM_16BIT;
				else
					pstState->insCurIns.bImmType = IMM_32BIT;
				pstState->dsNextState = DASM_STATE_IMM;
				break;

			case 0xea:	// EA cd JMP ptr16:16 or EA cp JMP ptr16:32
			{
				pstState->insCurIns.fCodeOffset = TRUE;
				if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
					pstState->insCurIns.bImmType = IMM_32BIT;
				else
					pstState->insCurIns.bImmType = IMM_48BIT;
				pstState->dsNextState = DASM_STATE_IMM;
				break;
			}

			case 0xff:
			{
				BYTE bOpcodeEx;
				Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				if(bOpcodeEx == 4)
				{
					if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
						pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
					else
						pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				}
				else if(bOpcodeEx == 5)
				{
					if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
						pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
					else
						pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
				}
				else
				{
					wprintf_s(L"OPCHndlrCC_JUMP(): Invalid opcode ex %d\n", bOpcodeEx);
					return FALSE;
				}
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
				break;
			}

			default:
				wprintf_s(L"OPCHndlrCC_JUMP(): Invalid opcode %xh\n", bOpcode);
				return FALSE;

		}// switch(bOpcode)
		
	}// if-else

	return TRUE;
}

/*	** TEST **
 * A8 ib	TEST AL,imm8	AND imm8 with AL; set SF, ZF, PF according to result
 * A9 iw	TEST AX,imm16	AND imm16 with AX; set SF, ZF, PF according to result
 * A9 id	TEST EAX,imm32	AND imm32 with EAX; set SF, ZF, PF according to result
 *
 * F6 /0 ib		TEST r/m8,imm8		AND imm8 with r/m8; set SF, ZF, PF according to result
 * F7 /0 iw		TEST r/m16,imm16	AND imm16 with r/m16; set SF, ZF, PF according to result
 * F7 /0 id		TEST r/m32,imm32	AND imm32 with r/m32; set SF, ZF, PF according to result
 *
 * 84 /r	TEST r/m8,r8	AND r8 with r/m8; set SF, ZF, PF according to result
 * 85 /r	TEST r/m16,r16	AND r16 with r/m16; set SF, ZF, PF according to result
 * 85 /r	TEST r/m32,r32	AND r32 with r/m32; set SF, ZF, PF according to result
 */
BOOL OPCHndlrCC_TEST(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	switch(bOpcode)
	{
		case 0xa8:
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;

			// Construct the output string
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes8[REGCODE_AL]);
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0xa9:
			pstState->insCurIns.fImm = TRUE;
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				pstState->insCurIns.bImmType = IMM_16BIT;
				// Construct the output string
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes16[REGCODE_AX]);
			}
			else
			{
				pstState->insCurIns.bImmType = IMM_32BIT;
				// Construct the output string
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes32[REGCODE_EAX]);
			}
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0xf6:	// w-bit and op size prefix can determine the operands' size
		case 0xf7:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0x84:	// w-bit and op size prefix can determine the operands' size
		case 0x85:
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_R;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		default:
			wprintf_s(L"OPCHndlrCC_TEST(): Invalid opcode %xh\n", bOpcode);
			return FALSE;

	}// switch(bOpcode)
	return TRUE;
}

/*	** CMPSx **
 * A6 CMPS m8, m8		Compares byte at address DS:(E)SI with byte at address
 *							ES:(E)DI and sets the status flags accordingly
 * A7 CMPS m16, m16		Compares word at address DS:(E)SI with word at address
 *							ES:(E)DI and sets the status flags accordingly
 * A7 CMPS m32, m32		Compares doubleword at address DS:(E)SI with doubleword
 *							at address ES:(E)DI and sets the status flags accordingly
 * A6 CMPSB				Compares byte at address DS:(E)SI with byte at address
 *							ES:(E)DI and sets the status flags accordingly
 * A7 CMPSW				Compares word at address DS:(E)SI with word at address
 *							ES:(E)DI and sets the status flags accordingly
 * A7 CMPSD				Compares doubleword at address DS:(E)SI with doubleword
 *							at address ES:(E)DI and sets the status flags accordingly
 */
BOOL OPCHndlrCC_CMPS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WCHAR *pwszxDI = NULL;
	WCHAR *pwszxSI = NULL;
	WCHAR *pwszPtrStr = NULL;
	
	// Determine DI/EDI based on address size attrib
	if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
	{
		pwszxDI = awszRegCodes16[REGCODE_DI];
		pwszxSI = awszRegCodes16[REGCODE_SI];
	}
	else
	{
		pwszxDI = awszRegCodes32[REGCODE_EDI];
		pwszxSI = awszRegCodes32[REGCODE_ESI];
	}

	// to print: cmps        byte ptr [esi],byte ptr es:[edi]
	if(bOpcode == 0xA6)
		pwszPtrStr = awszPtrStr[PTR_STR_INDEX_BYTE];
	else if(bOpcode == 0xA7)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_WORD];
		else
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_DWORD];
	}

	// destination: <ptrStr>[(e)si]
	StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
						L"%s[%s]", pwszPtrStr, pwszxSI);

	// source: <ptrStr> es:[(e)di]
	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
					L"%ses:[%s]", pwszPtrStr, pwszxDI);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** SCASx **
 * AE	SCAS m8		Compare AL with byte at ES:(E)DI and set status flags
 * AF	SCAS m16	Compare AX with word at ES:(E)DI and set status flags
 * AF	SCAS m32	Compare EAX with doubleword at ES(E)DI and set status flags
 * AE	SCASB		Compare AL with byte at ES:(E)DI and set status flags
 * AF	SCASW		Compare AX with word at ES:(E)DI and set status flags
 * AF	SCASD		Compare EAX with doubleword at ES:(E)DI and set status flags
 */
BOOL OPCHndlrCC_SCAS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WCHAR *pwszxDI = NULL;
	WCHAR *pwszPtrStr = NULL;
	
	// Determine DI/EDI based on address size attrib
	if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
		pwszxDI = awszRegCodes16[REGCODE_DI];
	else
		pwszxDI = awszRegCodes32[REGCODE_EDI];

	if(bOpcode == 0xAE)
		pwszPtrStr = awszPtrStr[PTR_STR_INDEX_BYTE];
	else if(bOpcode == 0xAF)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_WORD];
		else
			pwszPtrStr = awszPtrStr[PTR_STR_INDEX_DWORD];
	}

	// to print: scas   dword ptr es:[edi]
	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
					L"%ses:[%s]", pwszPtrStr, pwszxDI);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** RET **
 * C3		RET			Near return to calling procedure
 * CB		RET			Far return to calling procedure
 * C2 iw	RET imm16	Near return to calling procedure and pop imm16 bytes from stack
 * CA iw	RET imm16	Far return to calling procedure and pop imm16 bytes from stack
 */
BOOL OPCHndlrCC_RETN(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(bOpcode == 0xc3 || bOpcode == 0xcb)
	{
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else if(bOpcode == 0xc2 || bOpcode == 0xca)
	{
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_16BIT;
		pstState->dsNextState = DASM_STATE_IMM;
	}
	else
	{
		wprintf_s(L"OPCHndlrCC_RETN(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}
	return TRUE;
}


BOOL OPCHndlrCC_IRETD(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// "iret" if opsize = 16bit
	// else, "iretd" is already written to 
	// pstState->wszCurInsStr
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"iret");

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** LOOP/LOOPxx **
 * E2 cb	LOOP rel8		Decrement count; jump short if count != 0
 * E1 cb	LOOPE rel8		Decrement count; jump short if count != 0 and ZF=1
 * E1 cb	LOOPZ rel8		Decrement count; jump short if count != 0 and ZF=1
 * E0 cb	LOOPNE rel8		Decrement count; jump short if count != 0 and ZF=0
 * E0 cb	LOOPNZ rel8		Decrement count; jump short if count != 0 and ZF=0
 */
BOOL OPCHndlrCC_CLOOP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Offset handled in IMM state
	pstState->insCurIns.fCodeOffset = TRUE;
	pstState->insCurIns.bImmType = IMM_8BIT;
	pstState->dsNextState = DASM_STATE_IMM;

	return TRUE;
}


/*	** CALL **
 * E8 cw	CALL rel16		Call near, relative, displacement relative to next instruction
 * E8 cd	CALL rel32		Call near, relative, displacement relative to next instruction
 * FF /2	CALL r/m16		Call near, absolute indirect, address given in r/m16
 * FF /2	CALL r/m32		Call near, absolute indirect, address given in r/m32
 * 9A cd	CALL ptr16:16	Call far, absolute, address given in operand
 * 9A cp	CALL ptr16:32	Call far, absolute, address given in operand
 * FF /3	CALL m16:16		Call far, absolute indirect, address given in m16:16
 * FF /3	CALL m16:32		Call far, absolute indirect, address given in m16:32
 */
BOOL OPCHndlrCC_CALL(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	errno_t iError = ERROR_SUCCESS;

	switch(bOpcode)
	{

		case 0xe8:
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				// rel16
				// Handled in IMM state
				pstState->insCurIns.bImmType = IMM_16BIT;
			}
			else
			{
				// rel32
				pstState->insCurIns.bImmType = IMM_32BIT;
			}
			pstState->insCurIns.fCodeOffset = TRUE;
			pstState->dsNextState = DASM_STATE_IMM;
			break;

		case 0x9a:	// CALLF
			if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			{
				// ptr 16bit, offset 16bit
				WORD wPtr;
				short sOffset;

				wPtr = *((WORD*)pstState->pbCurrentCodePtr);
				pstState->pbCurrentCodePtr += sizeof(WORD);
				
				sOffset = *((short*)pstState->pbCurrentCodePtr);
				pstState->pbCurrentCodePtr += sizeof(short);

				StringCchPrintf(pstState->wszCurInsTempStr, _countof(pstState->wszCurInsTempStr), 
								L"word ptr [%04xh]:%04xh", wPtr, sOffset);
				iError = wcscat_s(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), pstState->wszCurInsTempStr);
				#ifdef _DEBUG
					if(iError)
						wprintf_s(L"OPCHndlrCC_CALL(): 3 wcscat_s() error %d\n", iError);
				#endif

			}
			else
			{
				// ptr 16bit, offset 32bit
				WORD wPtr;
				INT iOffset;

				wPtr = *((WORD*)pstState->pbCurrentCodePtr);
				pstState->pbCurrentCodePtr += sizeof(WORD);
				
				iOffset = *((INT*)pstState->pbCurrentCodePtr);
				pstState->pbCurrentCodePtr += sizeof(INT);

				StringCchPrintf(pstState->wszCurInsTempStr, _countof(pstState->wszCurInsTempStr), 
								L"word ptr [%04xh]:%08xh", wPtr, iOffset);
				iError = wcscat_s(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), pstState->wszCurInsTempStr);
				#ifdef _DEBUG
					if(iError)
						wprintf_s(L"OPCHndlrCC_CALL(): 3 wcscat_s() error %d\n", iError);
				#endif
			}

			pstState->dsNextState = DASM_STATE_DUMP;
			break;

		case 0xff:
			BYTE bReg;
			
			// Determine whether it is FF /2 or FF /3 call
			Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bReg, NULL);
			if(bReg == 2)
			{
				// CALL r/m16 or CALL r/m32
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			else if(bReg == 3)
			{
				// CALL m16:16 or CALL m16:32
				pstState->insCurIns.fSpecialInstruction = TRUE;
				if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				else
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			else
			{
				wprintf_s(L"OPCHndlrCC_CALL(): Invalid reg field value %d\n", bReg);
				return FALSE;
			}

			break;

		default:
			wprintf_s(L"OPCHndlrCC_CALL(): Invalid opcode %xh\n", bOpcode);
			return FALSE;

	}// switch(bOpcode)

	return TRUE;
}

/* ** EFLAGS **
 * F8	CLC		Clear CF flag
 * FC	CLD		Clear DF flag
 * FA	CLI		Clear Int flag
 * F5	CMC		Complement CF flag
 * F9	STC		Set CF flag
 * FD	STD		Set DF flag
 * FB	STI		Set interrupt flag; external, maskable interrupts enabled
 *				at the end of the next instruction
 */
BOOL OPCHndlrCC_EFLAGS(PDASMSTATE pstState, BYTE bOpcode)	// EFLAGS manipulators: CMC, CLC, STC, ...
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

// System and IO instructions

/*	** INSx **
 * 6C INS m8, DX	Input byte from I/O port specified in DX into memory location specified in ES:(E)DI
 * 6D INS m16, DX	Input word from I/O port specified in DX into memory location specified in ES:(E)DI
 * 6D INS m32, DX	Input doubleword from I/O port specified in DX into memory location specified in ES:(E)DI
 *
 * 6C INSB	Input byte from I/O port specified in DX into memory location specified with ES:(E)DI
 * 6D INSW	Input word from I/O port specified in DX into memory location specified in ES:(E)DI
 * 6D INSD	Input doubleword from I/O port specified in DX into memory location specified in ES:(E)DI
 */
BOOL OPCHndlrSysIO_INS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// 004011CC: 6D		ins		dword ptr es:[edi],dx

	WCHAR *pwszxDI = NULL;
	
	// Determine DI/EDI based on address size attrib
	if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
		pwszxDI = awszRegCodes16[REGCODE_DI];
	else
		pwszxDI = awszRegCodes32[REGCODE_EDI];

	if(bOpcode == 0x6C)
	{
		// byte ptr es:[(e)di]
		StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
						L"%ses:[%s]", awszPtrStr[PTR_STR_INDEX_BYTE], pwszxDI);
	}
	else if(bOpcode == 0x6D)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
		{
			// word ptr es:[(e)di]
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
							L"%ses:[%s]", awszPtrStr[PTR_STR_INDEX_WORD], pwszxDI);
		}
		else
		{
			// dword ptr es:[(e)di]
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
							L"%ses:[%s]", awszPtrStr[PTR_STR_INDEX_DWORD], pwszxDI);
		}
	}

	// source is always dx: port address is in dx
	StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc), 
					awszRegCodes16[REGCODE_DX]);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


/*	** OUTS **
 * 6E	OUTS DX, m8		Output byte from memory location specified in DS:(E)SI to I/O port specified in DX
 * 6F	OUTS DX, m16	Output word from memory location specified in DS:(E)SI to I/O port specified in DX
 * 6F	OUTS DX, m32	Output doubleword from memory location specified in DS:(E)SI to I/O port specified in DX
 */
BOOL OPCHndlrSysIO_OUTS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	WCHAR *pwszxDI = NULL;
	
	// Determine DI/EDI based on address size attrib
	if(pstState->insCurIns.wPrefixTypes & PREFIX_ADSIZE)
		pwszxDI = awszRegCodes16[REGCODE_DI];
	else
		pwszxDI = awszRegCodes32[REGCODE_EDI];

	if(bOpcode == 0x6E)
	{
		// byte ptr ds:[(e)di]
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
						L"%sds:[%s]", awszPtrStr[PTR_STR_INDEX_BYTE], pwszxDI);
	}
	else if(bOpcode == 0x6F)
	{
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
		{
			// word ptr ds:[(e)di]
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
							L"%sds:[%s]", awszPtrStr[PTR_STR_INDEX_WORD], pwszxDI);
		}
		else
		{
			// dword ptr ds:[(e)di]
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
							L"%sds:[%s]", awszPtrStr[PTR_STR_INDEX_DWORD], pwszxDI);
		}
	}

	// destination is always dx: port address is in dx
	StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes), 
					awszRegCodes16[REGCODE_DX]);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** WAIT **
 * 9B	WAIT	Check pending unmasked floating-point exceptions.
 * FWAIT is an alternate mnemonic.
 */
BOOL OPCHndlrSysIO_WAIT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** intx **
 * CC		INT 3		Interrupt 3—trap to debugger
 * CD ib	INT imm8	Interrupt vector number specified by immediate byte
 * CE		intO		Interrupt 4—if overflow flag is 1
 */

BOOL OPCHndlrSysIO_INT(PDASMSTATE pstState, BYTE bOpcode)	//	int3/intn/intO
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(bOpcode == 0xcc)
	{
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"int");
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, 
							_countof(pstState->insCurIns.wszCurInsStrSrc), L"3");
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else if(bOpcode == 0xcd)
	{
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
		pstState->dsNextState = DASM_STATE_IMM;
	}
	else if(bOpcode == 0xce)
	{
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else
	{
		wprintf_s(L"OPCHndlrSysIO_int(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	return TRUE;
}

BOOL OPCHndlrSysIO_IceBP(PDASMSTATE pstState, BYTE bOpcode)	// undocumented INT1
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;

	return TRUE;
}


/*	** IN imm **
 * E4 ib	IN AL,imm8		Input byte from imm8 I/O port address into AL
 * E5 ib	IN AX,imm8		Input byte from imm8 I/O port address into AX
 * E5 ib	IN EAX,imm8		Input byte from imm8 I/O port address into EAX
 */
BOOL OPCHndlrSysIO_IN(PDASMSTATE pstState, BYTE bOpcode)	// IN imm
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// w, opsize attrib
	if(bOpcode == 0xE4)
	{
		StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
							awszRegCodes8[REGCODE_AL]);
	}
	else if(bOpcode == 0xE5)
	{
		 if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes8[REGCODE_AX]);
		else
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes8[REGCODE_EAX]);
	}
	else
	{
		wprintf_s(L"OPCHndlrSysIO_IN(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->insCurIns.fImm = TRUE;
	pstState->insCurIns.bImmType = IMM_8BIT;
	pstState->dsNextState = DASM_STATE_IMM;
	return TRUE;
}

/*	** OUT **
 * E6 ib	OUT imm8, AL	Output byte in AL to I/O port address imm8
 * E7 ib	OUT imm8, AX	Output word in AX to I/O port address imm8
 * E7 ib	OUT imm8, EAX	Output doubleword in EAX to I/O port address imm8
 */
BOOL OPCHndlrSysIO_OUT(PDASMSTATE pstState, BYTE bOpcode)	// OUT imm
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	BYTE bPortAddr;

	// w, opsize attrib
	if(bOpcode == 0xE6)
	{
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
							awszRegCodes8[REGCODE_AL]);
	}
	else if(bOpcode == 0xE7)
	{
		 if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								awszRegCodes8[REGCODE_AX]);
		else
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								awszRegCodes8[REGCODE_EAX]);
	}
	else
	{
		wprintf_s(L"OPCHndlrSysIO_OUT(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}
	pstState->insCurIns.fSrcStrSet = TRUE;
	
	// The imm8 value is the source!
	// This is not handled in either IMM or DUMP state
	// so we will do it here instead
	bPortAddr = *pstState->pbCurrentCodePtr;
	++pstState->pbCurrentCodePtr;
	++pstState->nBytesCurIns;
	StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
					L"%Xh", bPortAddr);
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


/*	** IN xx,DX **
 * EC	IN AL,DX	Input byte from I/O port in DX into AL
 * ED	IN AX,DX	Input word from I/O port in DX into AX
 * ED	IN EAX,DX	Input doubleword from I/O port in DX into EAX
 */
BOOL OPCHndlrSysIO_INDX(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// w, opsize attrib
	if(bOpcode == 0xEC)
	{
		StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
							awszRegCodes8[REGCODE_AL]);
	}
	else if(bOpcode == 0xED)
	{
		 if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes8[REGCODE_AX]);
		else
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								awszRegCodes8[REGCODE_EAX]);
	}
	else
	{
		wprintf_s(L"OPCHndlrSysIO_INDX(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	// source is always DX: port address is in DX
	StringCchCopy(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
						awszRegCodes16[REGCODE_DX]);
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** OUT xx,DX **
 * EE	OUT DX, AL	Output byte in AL to I/O port address in DX
 * EF	OUT DX, AX	Output word in AX to I/O port address in DX
 * EF	OUT DX, EAX	Output doubleword in EAX to I/O port address in DX
 */
BOOL OPCHndlrSysIO_OUTDX(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// w, opsize attrib
	if(bOpcode == 0xEE)
	{
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
							awszRegCodes8[REGCODE_AL]);
	}
	else if(bOpcode == 0xEF)
	{
		 if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								awszRegCodes16[REGCODE_AX]);
		else
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								awszRegCodes32[REGCODE_EAX]);
	}
	else
	{
		wprintf_s(L"OPCHndlrSysIO_OUT(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}
	
	// Dest is always DX: port address is in DX
	StringCchCopy(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
					awszRegCodes16[REGCODE_DX]);
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

BOOL OPCHndlrSysIO_HLT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// Prefix handlers
BOOL OPCHndlrPrefix_Ovride(PDASMSTATE pstState, BYTE bOpcode)	// override prefixes
{
	// This function need not be called since prefixes are handled in 
	// PREFIX state
	#ifdef _DEBUG
		wprintf_s(L"!! %s(): %xh\n", __FUNCTIONW__, bOpcode);
		return TRUE;
	#endif

	return TRUE;
}

/*	** LOCK **
 * F0	LOCK	Asserts LOCK# signal for duration of the accompanying instruction
 */
BOOL OPCHndlrPrefix_LOCK(PDASMSTATE pstState, BYTE bOpcode)
{
	// This function need not be called since prefixes are handled in 
	// PREFIX state
	#ifdef _DEBUG
		wprintf_s(L"!! %s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif
	return TRUE;
}

BOOL OPCHndlrPrefix_CREP(PDASMSTATE pstState, BYTE bOpcode)	// conditional repetition: REPE/REPNE
{
	// This function need not be called since prefixes are handled in 
	// PREFIX state
	#ifdef _DEBUG
		wprintf_s(L"!! %s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// Opcodes that may mean any one of multiple instructions

/*
 * ADD	/0
 * OR	/1
 * ADC	/2
 * SBB	/3
 * AND	/4
 * SUB	/5
 * XOR	/6
 * CMP	/7
 */
BOOL OPCHndlrMulti_8x(PDASMSTATE pstState, BYTE bOpcode)		// 0x80 - 0x83
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Check the 'reg/opcode' field of the ModRM byte
	// This is the extension to the primary opcode.
	BYTE bOpcodeEx;

	static WCHAR awszMnemonics[][MAX_OPCODE_NAME_LEN+1] = {
					L"add",	L"or",	L"adc",	L"sbb", L"and", L"sub", L"xor", L"cmp" };

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);

	switch(bOpcodeEx)
	{
		case 0:	// add
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[0]);
			return OPCHndlrALU_ADD(pstState, bOpcode);

		case 2:	// adc
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[2]);
			return OPCHndlrALU_ADD(pstState, bOpcode);
		
		case 3:	// sbb
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[3]);
			return OPCHndlrALU_SUB(pstState, bOpcode);

		case 5:	// sub
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[5]);
			return OPCHndlrALU_SUB(pstState, bOpcode);
			
		case 1:	// or
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[1]);
			return OPCHndlrALU_OR(pstState, bOpcode);
			
		case 4:	// and
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[4]);
			return OPCHndlrALU_AND(pstState, bOpcode);

		case 6:	// xor
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[6]);
			return OPCHndlrALU_XOR(pstState, bOpcode);

		case 7:	// cmp
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[7]);
			return OPCHndlrCC_CMP(pstState, bOpcode);

		default:
			wprintf_s(L"OPCHndlrMulti_8x(): Invalid opcode extension %u\n", bOpcodeEx);
			return FALSE;
	}

	return TRUE;	// unreachable I know
}

/*
 *	** DIV **
 * F6 /6	DIV r/m8
 * F7 /6	DIV r/m16
 * F7 /6	DIV r/m32
 *	** IDIV **
 * F6 /7	IDIV r/m8
 * F7 /7	IDIV r/m16
 * F7 /7	IDIV r/m32
 *	** IMUL **
 * F6 /5 IMUL r/m8
 * F7 /5 IMUL r/m16
 * F7 /5 IMUL r/m32
 *	** MUL **
 * F6 /4	MUL r/m8
 * F7 /4	MUL r/m16
 * F7 /4	MUL r/m32
 *	** NEG **
 * F6 /3	NEG r/m8
 * F7 /3	NEG r/m16
 * F7 /3	NEG r/m32
 *	** NOT **
 * F6 /2	NOT r/m8
 * F7 /2	NOT r/m16
 * F7 /2	NOT r/m32
 *	** TEST **
 * F6 /0 ib		TEST r/m8,imm8
 * F7 /0 iw		TEST r/m16,imm16
 * F7 /0 id		TEST r/m32,imm32
 */
BOOL OPCHndlrMulti_fx(PDASMSTATE pstState, BYTE bOpcode)		// 0xf6 and 0xf7
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Check the 'reg/opcode' field of the ModRM byte
	// This is the extension to the primary opcode.
	BYTE bOpcodeEx;

	static WCHAR awszMnemonics[][MAX_OPCODE_NAME_LEN+1] = {
					L"test", L"", L"not", L"neg", L"mul", L"imul", L"div", L"idiv" };

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);

	switch(bOpcodeEx)
	{
		case 0:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[0]);
			return OPCHndlrCC_TEST(pstState, bOpcode);

		case 2:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[2]);
			return OPCHndlrALU_NOT(pstState, bOpcode);

		case 3:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[3]);
			return OPCHndlrALU_NEG(pstState, bOpcode);

		case 4:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[4]);
			return OPCHndlrALU_MUL(pstState, bOpcode);

		case 5:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[5]);
			return OPCHndlrALU_MUL(pstState, bOpcode);

		case 6:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[6]);
			return OPCHndlrALU_DIV(pstState, bOpcode);

		case 7:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[7]);
			return OPCHndlrALU_DIV(pstState, bOpcode);

		default:
			wprintf_s(L"OPCHndlrMulti_fx(): Invalid opcode extension %u\n", bOpcodeEx);
			return FALSE;
	}

	return TRUE;
}

/* 
 *	** INC **
 * FE /0	INC r/m8
 *	** DEC **
 * FE /1	DEC r/m8
 */
BOOL OPCHndlrMulti_IncDec(PDASMSTATE pstState, BYTE bOpcode)	// 0xfe: INC/DEC
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Check the 'reg/opcode' field of the ModRM byte
	// This is the extension to the primary opcode.
	BYTE bOpcodeEx;

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);
	
	if(bOpcodeEx == 0)
	{
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", L"inc");
		return OPCHndlrALU_INC(pstState, bOpcode);
	}
	else if(bOpcodeEx == 1)
	{
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", L"dec");
		return OPCHndlrALU_DEC(pstState, bOpcode);
	}
	else
	{
		wprintf_s(L"OPCHndlrMulti_IncDec(): Invalid opcode extension %u\n", bOpcodeEx);
		return FALSE;
	}

	return TRUE;	// unreachable
}

/*
 *	** CALL **
 * FF /2	CALL r/m16
 * FF /2	CALL r/m32
 * FF /3	CALL m16:16
 * FF /3	CALL m16:32
 *	** DEC **
 * FF /1	DEC r/m16
 * FF /1	DEC r/m32
 *	** INC **
 * FF /0	INC r/m16
 * FF /0	INC r/m32
 *	** JMP **
 * FF /4	JMP r/m16
 * FF /4	JMP r/m32
 * FF /5	JMP m16:16
 * FF /5	JMP m16:32
 *	** PUSH **
 * FF /6	PUSH r/m16
 * FF /6	PUSH r/m32
 */
BOOL OPCHndlrMulti_FF(PDASMSTATE pstState, BYTE bOpcode)		// 0xff
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Check the 'reg/opcode' field of the ModRM byte
	// This is the extension to the primary opcode.
	BYTE bOpcodeEx;

	static WCHAR awszMnemonics[][MAX_OPCODE_NAME_LEN+1] = {
					L"inc", L"dec", L"call", L"call", L"jmp", L"jmp", L"push" };

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);

	switch(bOpcodeEx)
	{
		case 0:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[0]);
			return OPCHndlrALU_INC(pstState, bOpcode);

		case 1:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[1]);
			return OPCHndlrALU_DEC(pstState, bOpcode);

		case 2:
		case 3:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[3]);
			return OPCHndlrCC_CALL(pstState, bOpcode);

		case 4:
		case 5:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[5]);
			return OPCHndlrCC_JUMP(pstState, bOpcode);

		case 6:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[6]);
			return OPCHndlrStack_PUSH(pstState, bOpcode);

		default:
			wprintf_s(L"OPCHndlrMulti_FF(): Invalid opcode extension %u\n", bOpcodeEx);
			return FALSE;
	}
	
	return TRUE; // unreachable
}


/*	** NOP **
 * 90	NOP
 * The NOP instruction is an alias mnemonic for the XCHG (E)AX, (E)AX instruction.
 */
BOOL OPCHndlr_NOP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** Multi-byte NOP **
 * 0F 1F /0 NOP r/m16 Multi-byte no-operation instruction.
 * 0F 1F /0 NOP r/m32 Multi-byte no-operation instruction.
 */
BOOL OPCHndlr_HNOP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Check if 'reg' is 0??

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


// Instruction I don't know yet
BOOL OPCHndlrUnKwn_SALC(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"OPCHndlrUnKwn_SALC(): %xh\n", bOpcode);
	#endif
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

BOOL OPCHndlrUnKwn_ICE(PDASMSTATE pstState, BYTE bOpcode)	// ?? SysIO
{
	#ifdef _DEBUG
		wprintf_s(L"OPCHndlrUnKwn_ICE(): %xh\n", bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*******************
 * FPU instructions
 *******************/

// FPU Basic Arithmetic

/*	** FADD **
 * D8 /0	FADD m32 real		Add m32real to ST(0) and store result in ST(0)
 * DC /0	FADD m64real		Add m64real to ST(0) and store result in ST(0)
 * D8 C0+i	FADD ST(0), ST(i)	Add ST(0) to ST(i) and store result in ST(0)
 * DC C0+i	FADD ST(i), ST(0)	Add ST(i) to ST(0) and store result in ST(i)
 * DE C0+i	FADDP ST(i), ST(0)	Add ST(0) to ST(i), store result in ST(i), and pop the register stack
 * DA /0	FIADD m32int		Add m32int to ST(0) and store result in ST(0)
 * DE /0	FIADD m16int		Add m16int to ST(0) and store result in ST(0)
 */
BOOL OPCHndlrFPU_FADD(PDASMSTATE pstState, BYTE bOpcode)	// FADDP/FIADD also
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Redundant code inside switch but I want to have only
	// one switch statement instead of multiple switch and if-else

	switch(bOpcode)
	{
		case 0xD8:
		{
			if(pstState->bFPUModRM >= 0xC0 && pstState->bFPUModRM <= 0xC7)	// D8 C0+i
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xC0);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// D8h

		case 0xDA:
		{
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->insCurIns.fSpecialInstruction = TRUE;
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		}// DAh

		case 0xDC:
		{
			if(pstState->bFPUModRM >= 0xC0 && pstState->bFPUModRM <= 0xC7)	// DC C0+i
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st(%d)", pstState->bFPUModRM - 0xC0);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;	
			}
			break;
		}// DCh
		
		case 0xDE:
		{
			if(pstState->bFPUModRM >= 0xC0 && pstState->bFPUModRM <= 0xC7)	// DE C0+i
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st(%d)", pstState->bFPUModRM - 0xC0);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// DEh

		default:
		{
			wprintf_s(L"OPCHndlrFPU_FADD(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
		}
	}// switch(bOpcode)

	return TRUE;
}


/*	** FSUB **
 * D8 /4	FSUB m32real		Subtract m32real from ST(0) and store result in ST(0)
 * DC /4	FSUB m64real		Subtract m64real from ST(0) and store result in ST(0)
 * D8 E0+i	FSUB ST(0), ST(i)	Subtract ST(i) from ST(0) and store result in ST(0)
 * DC E8+i	FSUB ST(i), ST(0)	Subtract ST(0) from ST(i) and store result in ST(i)
 * DE E8+i	FSUBP ST(i), ST(0)	Subtract ST(0) from ST(i), store result in ST(i), and pop register stack
 * DA /4	FISUB m32int		Subtract m32int from ST(0) and store result in ST(0)
 * DE /4	FISUB m16int		Subtract m16int from ST(0) and store result in ST(0)
 */
BOOL OPCHndlrFPU_FSUB(PDASMSTATE pstState, BYTE bOpcode)	// + FSUBP/FISUB/FSUBR/FSUBRP/FISUBR
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Redundant code inside switch but I want to have only
	// one switch statement instead of multiple switch and if-else

	switch(bOpcode)
	{
		case 0xD8:
		{
			if(pstState->bFPUModRM >= 0xE0 && pstState->bFPUModRM <= 0xE7)	// D8 E0+i
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xE0);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// D8h

		case 0xDA:
		{
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->insCurIns.fSpecialInstruction = TRUE;
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		}// DAh

		case 0xDC:
		{
			if(pstState->bFPUModRM >= 0xE8 && pstState->bFPUModRM <= 0xEF)	// DC E8+i
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xE8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// DCh
		
		case 0xDE:
		{
			if(pstState->bFPUModRM >= 0xE8 && pstState->bFPUModRM <= 0xEF)
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xE8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// DEh

		default:
		{
			wprintf_s(L"OPCHndlrFPU_FSUB(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
		}
	}// switch(bOpcode)

	return TRUE;
}


/*
 * D8 /1	FMUL m32real		Multiply ST(0) by m32real and store result in ST(0)
 * DC /1	FMUL m64real		Multiply ST(0) by m64real and store result in ST(0)
 * D8 C8+i	FMUL ST(0), ST(i)	Multiply ST(0) by ST(i) and store result in ST(0)
 * DC C8+i	FMUL ST(i), ST(0)	Multiply ST(i) by ST(0) and store result in ST(i)
 * DE C8+i	FMULP ST(i), ST(0)	Multiply ST(i) by ST(0), store result in ST(i), and pop the register stack.
 * DA /1	FIMUL m32int		Multiply ST(0) by m32int and store result in ST(0)
 * DE /1	FIMUL m16int		Multiply ST(0) by m16int and store result in ST(0)
 */
BOOL OPCHndlrFPU_FMUL(PDASMSTATE pstState, BYTE bOpcode)	// + FMULP/FIMUL
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Redundant code inside switch but I want to have only
	// one switch statement instead of multiple switch and if-else

	switch(bOpcode)
	{
		case 0xD8:
		{
			if(pstState->bFPUModRM >= 0xC8 && pstState->bFPUModRM <= 0xCF)
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xC8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// D8h

		case 0xDA:
		{
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->insCurIns.fSpecialInstruction = TRUE;
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		}// DAh

		case 0xDC:
		{
			if(pstState->bFPUModRM >= 0xC8 && pstState->bFPUModRM <= 0xCF)
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xC8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// DCh
		
		case 0xDE:
		{
			if(pstState->bFPUModRM >= 0xC8 && pstState->bFPUModRM <= 0xCF)
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xC8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// DEh

		default:
		{
			wprintf_s(L"OPCHndlrFPU_FMUL(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
		}
	}// switch(bOpcode)

	return TRUE;
}


/*
 * D8 /6	FDIV m32real		Divide ST(0) by m32real and store result in ST(0)
 * DC /6	FDIV m64real		Divide ST(0) by m64real and store result in ST(0)
 * D8 F0+i	FDIV ST(0), ST(i)	Divide ST(0) by ST(i) and store result in ST(0)
 * DC F8+i	FDIV ST(i), ST(0)	Divide ST(i) by ST(0) and store result in ST(i)
 *
 * DE F8+i	FDIVP ST(i), ST(0)	Divide ST(i) by ST(0), store result in ST(i), and pop the register stack
 *
 * DA /6	FIDIV m32int		Divide ST(0) by m32int and store result in ST(0)
 * DE /6	FIDIV m16int		Divide ST(0) by m16int and store result in ST(0)
 *
 * D8 /7	FDIVR m32real		Divide m32real by ST(0) and store result in ST(0)
 * DC /7	FDIVR m64real		Divide m64real by ST(0) and store result in ST(0)
 * D8 F8+i	FDIVR ST(0), ST(i)	Divide ST(i) by ST(0) and store result in ST(0)
 * DC F0+i	FDIVR ST(i), ST(0)	Divide ST(0) by ST(i) and store result in ST(i)
 * DE F0+i	FDIVRP ST(i), ST(0) Divide ST(0) by ST(i), store result in ST(i), and pop the register stack
 * DA /7	FIDIVR m32int		Divide m32int by ST(0) and store result in ST(0)
 * DE /7	FIDIVR m16int		Divide m16int by ST(0) and store result in ST(0)
 */
BOOL OPCHndlrFPU_FDIV(PDASMSTATE pstState, BYTE bOpcode)	// + FDIVR/FIDIV/FDIVP/FDIVRP/FIDIVR
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Redundant code inside switch but I want to have only
	// one switch statement instead of multiple switch and if-else

	switch(bOpcode)
	{
		case 0xD8:
		{
			if(pstState->bFPUModRM >= 0xF0 && pstState->bFPUModRM <= 0xF7)	// FDIV
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xF0);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else if(pstState->bFPUModRM >= 0xF8 && pstState->bFPUModRM <= 0xFF)	// FDIVR
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xF8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// D8h

		case 0xDA:
		{
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->insCurIns.fSpecialInstruction = TRUE;
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;
		}// DAh

		case 0xDC:
		{
			if(pstState->bFPUModRM >= 0xF0 && pstState->bFPUModRM <= 0xF7)	// FDIVR
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xF0);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else if(pstState->bFPUModRM >= 0xF8 && pstState->bFPUModRM <= 0xFF)	// FDIV
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xF8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// DCh
		
		case 0xDE:
		{
			if(pstState->bFPUModRM >= 0xF0 && pstState->bFPUModRM <= 0xF7)	// FDIVRP
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xF0);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else if(pstState->bFPUModRM >= 0xF8 && pstState->bFPUModRM <= 0xFF)	// FDIVP
			{
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", pstState->bFPUModRM - 0xF8);
				StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
								L"st");
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->insCurIns.fDesStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else
			{
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// DEh

		default:
		{
			wprintf_s(L"OPCHndlrFPU_FDIV(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
		}
	}// switch(bOpcode)

	return TRUE;
}


BOOL OPCHndlrFPU_FABS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


BOOL OPCHndlrFPU_FCHS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


BOOL OPCHndlrFPU_FSQRT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


BOOL OPCHndlrFPU_FPREM(PDASMSTATE pstState, BYTE bOpcode)	// + FPREM1
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


BOOL OPCHndlrFPU_FRNDINT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


BOOL OPCHndlrFPU_FXTRACT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}



// FPU Load Constants
/*
 * D9 E8	FLD1
 * D9 E9	FLDL2T
 * D9 EA	FLDL2E
 * D9 EB	FLDPI
 * D9 EC	FLDLG2	Push log10(2) onto the FPU register stack.
 * D9 ED	FLDLN2	Push loge(2) onto the FPU register stack.
 * D9 EE	FLDZ	Push +0.0 onto the FPU register stack.
 *
 */
BOOL OPCHndlrFPU_Const(PDASMSTATE pstState, BYTE bOpcode)	// all constants
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}



// FPU Data Transfer

/*
 * D9 /0	FLD m32real		Push m32real onto the FPU register stack.
 * DD /0	FLD m64real		Push m64real onto the FPU register stack.
 * DB /5	FLD m80real		Push m80real onto the FPU register stack.
 * D9 C0+i	FLD ST(i)		Push ST(i) onto the FPU register stack.
 *
 * DF /0	FILD m16int		Push m16int onto the FPU register stack.
 * DB /0	FILD m32int		Push m32int onto the FPU register stack.
 * DF /5	FILD m64int		Push m64int onto the FPU register stack.
 *
 * DF /4	FBLD m80dec
 */
BOOL OPCHndlrFPU_FLoad(PDASMSTATE pstState, BYTE bOpcode)		// FLD/FILD/FBLD
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif

	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(pstState->bFPUModRM >= 0xC0 && pstState->bFPUModRM <= 0xC7)
	{
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
						L"st(%d)", pstState->bFPUModRM - 0xC0);
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else
	{
		pstState->insCurIns.fSpecialInstruction = TRUE;
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
		pstState->dsNextState = DASM_STATE_MOD_RM;
		switch(bOpcode)
		{
			case 0xDF:	// 16bit/64bit/80bit
			{
				if(pstState->bFPUReg == 0)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
				else if(pstState->bFPUReg == 5)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
				else if(pstState->bFPUReg == 4)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_80BIT;
				else
				{
					wprintf_s(L"OPCHndlrFPU_FLoad(): Invalid 'reg' field %xh\n", pstState->bFPUReg);
					return FALSE;
				}
				break;
			}

			case 0xD9:	// m32real
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				break;
			}

			case 0xDD:	// m64real
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
				break;
			}

			case 0xDB:
			{
				if(pstState->bFPUReg == 0)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				else if(pstState->bFPUReg == 5)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_80BIT;
				else
				{
					wprintf_s(L"OPCHndlrFPU_FLoad(): Invalid 'reg' field %xh\n", pstState->bFPUReg);
					return FALSE;
				}
				break;
			}

			default:
			{
				wprintf_s(L"OPCHndlrFPU_FStore(): Invalid opcode %xh\n", bOpcode);
				return FALSE;
			}

		}// switch(bOpcode)
	}

	return TRUE;
}


/*	** Store **
 * D9 /2	FST m32real		Copy ST(0) to m32real
 * DD /2	FST m64real		Copy ST(0) to m64real
 * DD D0+i	FST ST(i)		Copy ST(0) to ST(i)
 * D9 /3	FSTP m32real	Copy ST(0) to m32real and pop register stack
 * DD /3	FSTP m64real	Copy ST(0) to m64real and pop register stack
 * DB /7	FSTP m80real	Copy ST(0) to m80real and pop register stack
 * DD D8+i	FSTP ST(i)		Copy ST(0) to ST(i) and pop register stack
 *
 * DF /2 FIST m16int	Store ST(0) in m16int
 * DB /2 FIST m32int	Store ST(0) in m32int
 * DF /3 FISTP m16int	Store ST(0) in m16int and pop register stack
 * DB /3 FISTP m32int	Store ST(0) in m32int and pop register stack
 * DF /7 FISTP m64int	Store ST(0) in m64int and pop register stack
 *
 * DF /6 FBSTP m80bcd	Store ST(0) in m80bcd and pop ST(0).
 */
BOOL OPCHndlrFPU_FStore(PDASMSTATE pstState, BYTE bOpcode)		// FST/FSTP/FIST/FISTP/FBSTP
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(pstState->bFPUModRM >= 0xD0 && pstState->bFPUModRM <= 0xD7)
	{
		// FST st(i)
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
						L"st(%d)", pstState->bFPUModRM - 0xD0);
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else if(pstState->bFPUModRM >= 0xD8 && pstState->bFPUModRM <= 0xDF)
	{
		// FSTP st(i)
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
						L"st(%d)", pstState->bFPUModRM - 0xD8);
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
	}
	else
	{
		pstState->insCurIns.fSpecialInstruction = TRUE;
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
		pstState->dsNextState = DASM_STATE_MOD_RM;
		switch(bOpcode)
		{
			case 0xD9:
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				break;

			case 0xDD:
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
				break;

			case 0xDB:
				if(pstState->bFPUReg == 7)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_80BIT;
				else	// DB /2 and DB /3
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;

				break;

			case 0xDF:
				if(pstState->bFPUReg == 7)
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
				else if(pstState->bFPUReg == 6)	// fbstp /6
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_80BIT;
				else	// DF /2 and DF /3
					pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
				break;

			default:
			{
				wprintf_s(L"OPCHndlrFPU_FStore(): Invalid opcode %xh\n", bOpcode);
				return FALSE;
			}
			
		}// switch(pstState->bFPUReg)
	}

	return TRUE;
}

/*	** FXCH **
 * D9 C8+i	FXCH ST(i)	Exchange the contents of ST(0) and ST(i)
 * D9 C9	FXCH		Exchange the contents of ST(0) and ST(1)
 */
BOOL OPCHndlrFPU_FXCH(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	ASSERT(pstState->bFPUModRM >= 0xC8 && pstState->bFPUModRM <= 0xCF);	// always??

	BYTE bFPUDataReg = pstState->bFPUModRM - 0xC8;

	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
					L"st(%x)", bFPUDataReg);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


/*	** FCMOVcc **
 * DA C0+i FCMOVB ST(0), ST(i)		Move if below (CF=1)
 * DA C8+i FCMOVE ST(0), ST(i)		Move if equal (ZF=1)
 * DA D0+i FCMOVBE ST(0), ST(i)		Move if below or equal (CF=1 or ZF=1)
 * DA D8+i FCMOVU ST(0), ST(i)		Move if unordered (PF=1)
 * DB C0+i FCMOVNB ST(0), ST(i)		Move if not below (CF=0)
 * DB C8+i FCMOVNE ST(0), ST(i)		Move if not equal (ZF=0)
 * DB D0+i FCMOVNBE ST(0), ST(i)	Move if not below or equal (CF=0 and ZF=0)
 * DB D8+i FCMOVNU ST(0), ST(i)		Move if not unordered (PF=0)
 */
BOOL OPCHndlrFPU_FCMOV(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	BYTE bSrcReg;

	// Determine the source FPU data register
	if(pstState->bFPUModRM < 0xC8)				// C0+i
		bSrcReg = pstState->bFPUModRM - 0xC0;
	else if(pstState->bFPUModRM < 0xD0)			// C8+i
		bSrcReg = pstState->bFPUModRM - 0xC8;
	else if(pstState->bFPUModRM < 0xD8)			// D0+i
		bSrcReg = pstState->bFPUModRM - 0xD0;
	else								// D8+i
		bSrcReg = pstState->bFPUModRM - 0xD8;


	// source is st(i) and destination is st always
	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
					L"st(%d)", bSrcReg);
	StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
					L"st");
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->insCurIns.fDesStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}



// FPU Compare/Classify

/*	** FCOM **
 * D8 /2	FCOM m32real	Compare ST(0) with m32real.
 * DC /2	FCOM m64real	Compare ST(0) with m64real.
 * D8 D0+i	FCOM ST(i)		Compare ST(0) with ST(i).
 * D8 /3	FCOMP m32real	Compare ST(0) with m32real and pop register stack.
 * DC /3	FCOMP m64real	Compare ST(0) with m64real and pop register stack.
 * D8 D8+i	FCOMP ST(i)		Compare ST(0) with ST(i) and pop register stack.
 * DE D9	FCOMPP			Compare ST(0) with ST(1) and pop register stack twice.
 *
 * DB F0+i FCOMI ST, ST(i) Compare ST(0) with ST(i) and set status flags accordingly
 * DF F0+i FCOMIP ST, ST(i) Compare ST(0) with ST(i), set status flags accordingly, and pop register stack
 * DB E8+i FUCOMI ST, ST(i) Compare ST(0) with ST(i), check for ordered values, and set status flags accordingly
 * DF E8+i FUCOMIP ST, ST(i) Compare ST(0) with ST(i), check for ordered values, set status flags accordingly, and pop register stack
 *
 * DD E0+i	FUCOM ST(i)		Compare ST(0) with ST(i)
 * DD E8+i	FUCOMP ST(i)	Compare ST(0) with ST(i) and pop register stack
 * DA E9	FUCOMPP			Compare ST(0) with ST(1) and pop register stack twice
 */
BOOL OPCHndlrFPU_FCmpReal(PDASMSTATE pstState, BYTE bOpcode)	// FCOM,P,PP/FUCOM,P,PP/FCOMI,IP
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	BYTE bSrcReg;

	switch(bOpcode)
	{
		case 0xDA:
			pstState->dsNextState = DASM_STATE_DUMP;
			break;

		case 0xD8:
		{
			if(pstState->bFPUModRM >= 0xD0 && pstState->bFPUModRM <= 0xD7)		// D0+i
			{
				bSrcReg = pstState->bFPUModRM - 0xD0;
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", bSrcReg);
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else if(pstState->bFPUModRM >= 0xD8 && pstState->bFPUModRM <= 0xDF)	// D8+i
			{
				bSrcReg = pstState->bFPUModRM - 0xD8;
				StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								L"st(%d)", bSrcReg);
				pstState->insCurIns.fSrcStrSet = TRUE;
				pstState->dsNextState = DASM_STATE_DUMP;
			}
			else //if(pstState->bFPUReg == 2 || pstState->bFPUReg == 3)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				pstState->insCurIns.fSpecialInstruction = TRUE;
				pstState->insCurIns.fModRM = TRUE;
				pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
				pstState->dsNextState = DASM_STATE_MOD_RM;
			}
			break;
		}// D8h

		case 0xDB:
		case 0xDF:
		{
			if(pstState->bFPUModRM <= 0xF7)	// F0+i
				bSrcReg = pstState->bFPUModRM - 0xF0;
			else					// E8+i
				bSrcReg = pstState->bFPUModRM - 0xE8;

			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
							L"st(%d)", bSrcReg);
			StringCchPrintf(pstState->insCurIns.wszCurInsStrDes, _countof(pstState->insCurIns.wszCurInsStrDes),
							L"st");
			pstState->insCurIns.fSrcStrSet = TRUE;
			pstState->insCurIns.fDesStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_DUMP;
			break;
		}// DBh

		case 0xDC:
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
			pstState->insCurIns.fSpecialInstruction = TRUE;
			pstState->insCurIns.fModRM = TRUE;
			pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 0xDD:
		{
			if(pstState->bFPUModRM <= 0xE7)	// E0+i
				bSrcReg = pstState->bFPUModRM - 0xE0;
			else					// E8+i
				bSrcReg = pstState->bFPUModRM - 0xE8;
			StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
							L"st(%d)", bSrcReg);
			pstState->insCurIns.fSrcStrSet = TRUE;
			pstState->dsNextState = DASM_STATE_DUMP;
			break;
		}// DDh

		case 0xDE:
			pstState->dsNextState = DASM_STATE_DUMP;
			break;

		default:
			wprintf_s(L"OPCHndlrFPU_FCmpReal(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
	}

	return TRUE;
}

/*	** Compare Int **
 * DE /2	FICOM m16int	Compare ST(0) with m16int
 * DA /2	FICOM m32int	Compare ST(0) with m32int
 * DE /3	FICOMP m16int	Compare ST(0) with m16int and pop stack register
 * DA /3	FICOMP m32int	Compare ST(0) with m32int and pop stack register
 */
BOOL OPCHndlrFPU_FCmpInts(PDASMSTATE pstState, BYTE bOpcode)	// FICOM,P
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	/*
	 * Opcode DE has 16bit int memory operand
	 * Opcode DA has 32bit int memory operand
	 */
	if(bOpcode == 0xDE)
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
	else if(bOpcode == 0xDA)
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
	else
	{
		wprintf_s(L"OPCHndlrFPU_FCmpInts(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*	** FTST **
 * D9 E4	FTST	Compare ST(0) with 0.0.
 */
BOOL OPCHndlrFPU_FTST(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif

	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


/*	** FXAM **
 * D9 E5	FXAM	Classify value or number in ST(0)
 */
BOOL OPCHndlrFPU_FXAM(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}



// FPU Trigonometric

/*	** FPU Trigonometric Instructions *
 * D9 F2	FPTAN	Replace ST(0) with its tangent and push 1 onto the FPU stack.
 * D9 F3	FPATAN	Replace ST(1) with arctan(ST(1)/ST(0)) and pop the register stack.
 * D9 FB	FSINCOS Compute the sine and cosine of ST(0); replace ST(0) 
 *					with the sine, and push the cosine onto the register stack.
 * D9 FE	FSIN	Replace ST(0) with its sine.
 * D9 FF	FCOS	Replace ST(0) with its cosine
 */
BOOL OPCHndlrFPU_Trig(PDASMSTATE pstState, BYTE bOpcode)	// FSIN/FCOS/FSINCOS/FPTAN/FPATAN
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// FPU Log, Exp, Scale
/*
 * D9 F0	F2XM1	Replace ST(0) with (2^ST(0) – 1)
 * D9 F1	FYL2X
 * D9 F9	FYL2XP1
 * D9 FD	FSCALE	Scale ST(0) by ST(1).
 */
BOOL OPCHndlrFPU_LgExSc(PDASMSTATE pstState, BYTE bOpcode)	// FYL2X/FYL2XP1/F2XM1/FSCALE
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// FPU Control

/* Control instructions that have no operands
 *
 * DB E3	FNINIT
 * DB E2	FNCLEX
 * D9 F7	FINCSTP
 * D9 F6	FDECSTP
 * 
 */
BOOL OPCHndlrFPU_Ctl(PDASMSTATE pstState, BYTE bOpcode)		// No operands
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


/* Control instructions that have an operand
 *
 * D9 /5	FLDCW m2byte
 * D9 /7	FNSTCW m2byte
 * DD /7	FNSTSW m2byte
 * DF E0	FNSTSW AX
 * D9 /4	FLDENV m14/28byte
 * D9 /6	FNSTENV m14/28byte
 * DD /4	FRSTOR m94/108byte
 * DD /6	FNSAVE m94/108byte
 * DD C0+i	FFREE ST(i)
 */
BOOL OPCHndlrFPU_CtlOp(PDASMSTATE pstState, BYTE bOpcode)	// With operands
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// First, handle instructions that do not have a memory operand
	// FNSTSW
	if(pstState->bFPUModRM == 0xE0)
	{
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
						awszRegCodes16[REGCODE_AX]);
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
		return TRUE;
	}

	// FFREE
	if(pstState->bFPUModRM >= 0xC0 && pstState->bFPUModRM <= 0xC7)
	{
		BYTE bFPUDataReg = pstState->bFPUModRM - 0xC0;
		StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
						L"st(%d)", bFPUDataReg);
		pstState->insCurIns.fSrcStrSet = TRUE;
		pstState->dsNextState = DASM_STATE_DUMP;
		return TRUE;
	}

	// There is an operand in the ModRM byte
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	pstState->insCurIns.fSpecialInstruction = TRUE;

	// Determine operand size. Observe that only instructions with
	// 5 & 7 in the 'reg' field have m2byte operands. We do not 
	// display a PtrStr for other operand sizes.
	if(pstState->bFPUReg == 5 || pstState->bFPUReg == 7)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
	}
	// else pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_UNDEF;	// default
		
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


BOOL OPCHndlrFPU_FNOP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


BOOL OPCHndlrFPU_Invalid(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef UNIT_TEST_ONLY
		wprintf_s(L"%-25s : %02X %02X\n", __FUNCTIONW__, bOpcode, pstState->bFPUModRM);
	#endif
	
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	wprintf_s(L"OPCHndlrFPU_Invalid(): Invalid FPU opcode:modrm %xh:%xh\n", bOpcode, pstState->bFPUModRM);
	return FALSE;
}



/**********************************************
 * 0F xx opcodes
 *********************************************/

 // Arithmetic instructions

/*	** XADD **
 * 0F C0	/r	XADD r/m8,r8	Exchange r8 and r/m8; load sum into r/m8.
 * 0F C1	/r	XADD r/m16,r16	Exchange r16 and r/m16; load sum into r/m16.
 * 0F C1	/r	XADD r/m32,r32	Exchange r32 and r/m32; load sum into r/m32.
 */
BOOL OPCHndlrALU_XADD(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// operand size can be determined by w-bit and op size attribute
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


// Logical instructions


// Memory

/*	** LAR ** Load Access Rights
 * 0F 02 /r		LAR r16,r/m16	r16 = r/m16 masked by FF00H
 * 0F 02 /r		LAR r32,r/m32	r32 = r/m32 masked by 00FxFF00H
 */
BOOL OPCHndlrMem_LAR(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** LSL **
 * 0F 03 /r		LSL r16,r/m16	Load: r16 = segment limit, selector r/m16
 * 0F 03 /r		LSL r32,r/m32	Load: r32 = segment limit, selector r/m32)
 */
BOOL OPCHndlrMem_LSL(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// opsize attrib
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** MOV to/from control registers **
 * 0F 22 /r		MOV CR0,r32
 * 0F 22 /r		MOV CR2,r32
 * 0F 22 /r		MOV CR3,r32
 * 0F 22 /r		MOV CR4,r32
 * 0F 20 /r		MOV r32,CR0
 * 0F 20 /r		MOV r32,CR2
 * 0F 20 /r		MOV r32,CR3
 * 0F 20 /r		MOV r32,CR4
 *
 * 0F 21 /r		MOV r32, DR0-DR7
 * 0F 23 /r		MOV DR0-DR7,r32
 */
BOOL OPCHndlrMem_MOVCrDr(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(bOpcode == 0x22)
	{
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;

		pstState->insCurIns.fSpecialInstruction = TRUE;
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
		pstState->insCurIns.bRegTypeSrc = REGTYPE_GREG;
		pstState->insCurIns.bRegTypeDest = REGTYPE_CREG;
		pstState->dsNextState = DASM_STATE_MOD_RM;
	}
	else if(bOpcode == 0x20)
	{
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;

		pstState->insCurIns.fSpecialInstruction = TRUE;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
		pstState->insCurIns.bRegTypeSrc = REGTYPE_CREG;
		pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;
		pstState->dsNextState = DASM_STATE_MOD_RM;
	}
	else if(bOpcode == 0x21)
	{
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;

		pstState->insCurIns.fSpecialInstruction = TRUE;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
		pstState->insCurIns.bRegTypeSrc = REGTYPE_DREG;
		pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;
		pstState->dsNextState = DASM_STATE_MOD_RM;
	}
	else if(bOpcode == 0x23)
	{
		pstState->insCurIns.fModRM = TRUE;
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;

		pstState->insCurIns.fSpecialInstruction = TRUE;
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
		pstState->insCurIns.bRegTypeSrc = REGTYPE_GREG;
		pstState->insCurIns.bRegTypeDest = REGTYPE_DREG;
		pstState->dsNextState = DASM_STATE_MOD_RM;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_MOVCrDr(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** BT **
 * 0F A3	BT r/m16,r16	Store selected bit in CF flag
 * 0F A3	BT r/m32,r32	Store selected bit in CF flag
 * IASD Vol2 Table B-10: ModRM type = /r
 *
 * 0F BA /4 ib	BT r/m16,imm8	Store selected bit in CF flag
 * 0F BA /4 ib	BT r/m32,imm8	Store selected bit in CF flag
 */
BOOL OPCHndlrMem_BT(PDASMSTATE pstState, BYTE bOpcode)			// Bit Test
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// There is a modRM byte in any case
	pstState->insCurIns.fModRM = TRUE;

	if(bOpcode == 0xa3)
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	else if(bOpcode == 0xba)
	{
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_BT(): Invalid opcode %xh\n");
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** BTS **
 * 0F AB	BTS r/m16,r16	Store selected bit in CF flag and set
 * 0F AB	BTS r/m32,r32	Store selected bit in CF flag and set
 * IASD Vol2 Table B-10: ModRM type = /r
 *
 * 0F BA /5 ib	BTS r/m16,imm8 Store selected bit in CF flag and set
 * 0F BA /5 ib	BTS r/m32,imm8 Store selected bit in CF flag and set
 */
BOOL OPCHndlrMem_BTS(PDASMSTATE pstState, BYTE bOpcode)			// Bit Test and Set
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// There is a modRM byte in any case
	pstState->insCurIns.fModRM = TRUE;

	if(bOpcode == 0xab)
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	else if(bOpcode == 0xba)
	{
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_BTS(): Invalid opcode %xh\n");
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** LSS **
 * 0F B2 /r		LSS r16,m16:16	Load SS:r16 with far pointer from memory
 * 0F B2 /r		LSS r32,m16:32	Load SS:r32 with far pointer from memory
 */
BOOL OPCHndlrMem_LSS(PDASMSTATE pstState, BYTE bOpcode)			// Load Full Pointer
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	
	pstState->insCurIns.fSpecialInstruction = TRUE;
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	}
	else
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** BTR **
 * 0F B3	BTR r/m16,r16	Store selected bit in CF flag and clear
 * 0F B3	BTR r/m32,r32	Store selected bit in CF flag and clear
 * IASD Vol2 Table B-10: ModRM byte is present and is of type /r
 *
 * 0F BA /6 ib	BTR r/m16,imm8 Store selected bit in CF flag and clear
 * 0F BA /6 ib	BTR r/m32,imm8 Store selected bit in CF flag and clear
 */
BOOL OPCHndlrMem_BTR(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// There is a modRM byte in any case
	pstState->insCurIns.fModRM = TRUE;

	if(bOpcode == 0xb3)
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	else if(bOpcode == 0xba)
	{
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_BTR(): Invalid opcode %xh\n");
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** LFS **
 * 0F B4 /r		LFS r16,m16:16	Load FS:r16 with far pointer from memory
 * 0F B4 /r		LFS r32,m16:32	Load FS:r32 with far pointer from memory
 */
BOOL OPCHndlrMem_LFS(PDASMSTATE pstState, BYTE bOpcode)			// Load Full Pointer
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;

	pstState->insCurIns.fSpecialInstruction = TRUE;
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	}
	else
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** LGS **
 * 0F B5 /r		LGS r16,m16:16	Load GS:r16 with far pointer from memory
 * 0F B5 /r		LGS r32,m16:32	Load GS:r32 with far pointer from memory
 */
BOOL OPCHndlrMem_LGS(PDASMSTATE pstState, BYTE bOpcode)			// Load Full Pointer
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;

	pstState->insCurIns.fSpecialInstruction = TRUE;
	if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
	}
	else
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** MOVZX **
 * 0F B6 /r		MOVZX r16,r/m8		Move byte to word with zero-extension
 * 0F B6 /r		MOVZX r32,r/m8		Move byte to doubleword, zero-extension
 * 0F B7 /r		MOVZX r32,r/m16		Move word to doubleword, zero-extension
 */
BOOL OPCHndlrMem_MOVZX(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.fSpecialInstruction = TRUE;	// different sized source and dest operands
	if(bOpcode == 0xb6)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_8BIT;
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
		else
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}
	else if(bOpcode == 0xb7)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_MOVSX(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


/*	** MOVSX **
 * 0F BE /r		MOVSX r16,r/m8		Move byte to word with sign-extension
 * 0F BE /r		MOVSX r32,r/m8		Move byte to doubleword, sign-extension
 * 0F BF /r		MOVSX r32,r/m16		Move word to doubleword, sign-extension
 */
BOOL OPCHndlrMem_MOVSX(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.fSpecialInstruction = TRUE;	// different sized source and dest operands
	if(bOpcode == 0xbe)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_8BIT;
		if(pstState->insCurIns.wPrefixTypes & PREFIX_OPSIZE)
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_16BIT;
		else
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}
	else if(bOpcode == 0xbf)
	{
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
		pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_MOVSX(): Invalid opcode %xh\n", bOpcode);
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** BTC **
 * 0F BB	BTC r/m16,r16	Store selected bit in CF flag and complement
 * 0F BB	BTC r/m32,r32	Store selected bit in CF flag and complement
 * IASD Vol2 Table B-10: ModRM byte is present and is of type /r
 *
 * 0F BA /7 ib	BTC r/m16,imm8	Store selected bit in CF flag and complement
 * 0F BA /7 ib	BTC r/m32,imm8	Store selected bit in CF flag and complement
 */
BOOL OPCHndlrMem_BTC(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// There is a modRM byte in any case
	pstState->insCurIns.fModRM = TRUE;

	if(bOpcode == 0xbb)
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	else if(bOpcode == 0xba)
	{
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	}
	else
	{
		wprintf_s(L"OPCHndlrMem_BTC(): Invalid opcode %xh\n");
		return FALSE;
	}

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** BSF **
 * 0F BC	BSF r16,r/m16	Bit scan forward on r/m16
 * 0F BC	BSF r32,r/m32	Bit scan forward on r/m32
 * IASD Vol2 Table B-10: ModRM byte is present and is of type /r
 */
BOOL OPCHndlrMem_BSF(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Destination is always a register but the opcode has d-bit = 0
	// So force d-bit=1
	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fWBitAbsent = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** BSR **
 * 0F BD	BSR r16,r/m16	Bit scan reverse on r/m16
 * 0F BD	BSR r32,r/m32	Bit scan reverse on r/m32
 * IASD Vol2 Table B-10: ModRM byte is present and is of type /r
 */
BOOL OPCHndlrMem_BSR(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** BSWAP **
 * 0F C8+rd		BSWAP	r32 Reverses the byte order of a 32-bit register.
 */
BOOL OPCHndlrMem_ByteSWAP(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Get the register name index
	BYTE bReg = bOpcode - 0xC8;

	StringCchPrintf(pstState->insCurIns.wszCurInsStrSrc, _countof(pstState->insCurIns.wszCurInsStrSrc),
								awszRegCodes32[bReg]);
	pstState->insCurIns.fSrcStrSet = TRUE;
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// Control flow and Conditional

/* ** CMOVxx **
 * All have ModRM bytes and mnemonics are already written into
 * pstState->wszCurInsStr.
 * Operand size is 16/32bit depending on operandsize prefix.
 */
BOOL OPCHndlrCC_CMOV(PDASMSTATE pstState, BYTE bOpcode)		// Conditional move
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** SETxx **
 * IASD does not indicate the presence of a ModRM byte for these instructions
 * but the disassembly by dumpbin shows the usage of the ModRM byte and it
 * appears to be using only the Mod+RM bits.
 * Examples:
 *   00401129: 0F 9F 45 EB        setg        byte ptr [ebp-15h]
 *   0040112D: 0F 9F C0           setg        al
 *   00401130: 0F 9F C1           setg        cl
 *   00401133: 0F 92 C1           setb        cl
 *
 * Update: Table B-10 IASD Vol2 shows this is true. Reg = 000.
 */
BOOL OPCHndlrCC_SETxx(PDASMSTATE pstState, BYTE bOpcode)	// SET byte on condition
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;	// do not consider reg bits

	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_8BIT;	// 8bit always

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


/*	** CMPXCHG **
 * 0F B0 /r		CMPXCHG r/m8,r8		Compare AL with r/m8. If equal, ZF is set and r8 is loaded
 *									into r/m8. Else, clear ZF and load r/m8 into AL.
 * 0F B1 /r		CMPXCHG r/m16,r16	Compare AX with r/m16. If equal, ZF is set and r16 is
 *									loaded into r/m16. Else, clear ZF and load r/m16 into AL
 * 0F B1 /r		CMPXCHG r/m32,r32	Compare EAX with r/m32. If equal, ZF is set and r32 is
 *									loaded into r/m32. Else, clear ZF and load r/m32 into AL
 *
 * 0F C7 /1 m64		CMPXCHG8B m64	Compare EDX:EAX with m64. If equal, set ZF and load
 *									ECX:EBX into m64. Else, clear ZF and load m64 into
 *									EDX:EAX.
 */
BOOL OPCHndlrCC_CmpXchg(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	if(bOpcode == 0xb0 || bOpcode == 0xb1)
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	else if(bOpcode == 0xc7)
	{
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
		pstState->insCurIns.fSpecialInstruction = TRUE;
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
	}

	pstState->insCurIns.fModRM = TRUE;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}


// Sys or IO instructions

/*	** **
 * 0F 00 /2		LLDT r/m16		Load segment selector r/m16 into LDTR
 * 0F 00 /3		LTR r/m16		Load r/m16 into task register
 * 0F 00 /0		SLDT r/m16		Stores segment selector from LDTR in r/m16
 * 0F 00 /0		SLDT r/m32		Store segment selector from LDTR in low-order 16 bits of r/m32
 * 0F 00 /1		STR r/m16		Stores segment selector from TR in r/m16
 * 0F 00 /4		VERR r/m16		Set ZF=1 if segment specified with r/m16 can be read
 * 0F 00 /5		VERW r/m16		Set ZF=1 if segment specified with r/m16 can be written
 */
BOOL OPCHndlrSysIO_LdtTrS(PDASMSTATE pstState, BYTE bOpcode)	// 0f 00 LLDT/SLDT/LTR/...
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	BYTE bReg;
	static WCHAR awszMnemonics[][MAX_OPCODE_NAME_LEN+1] = 
					{L"sldt", L"str", L"lldt", L"ltr", L"verr", L"verw"};

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bReg, NULL);
	ASSERT(bReg >= 0 && bReg <= 5);	// todo: ASSERT_ALWAYS?
	StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[bReg]);

	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;	// even SLDT uses only 16bits
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** **
 * 0F 01 /6		LMSW r/m16		Loads r/m16 in machine status word of CR0
 * 0F 01 /7		INVLPG m		Invalidate TLB Entry for page that contains m;
 *								m is a 16/32bit operand in memory. (opsize attrib may be used).
 * 0F 01 /4		SMSW r/m16		Store machine status word to r/m16
 * 0F 01 /4		SMSW r32/m16	Store machine status word in low-order 16 bits of r32/m16; high-order 16 bits of r32 are undefined
 *
 * ** The destination operand specifies a 6-byte memory location **
 * 0F 01 /0		SGDT m			Store GDTR to m
 * 0F 01 /1		SIDT m			Store IDTR to m
 * 0F 01 /2		LGDT m16&32		Load m into GDTR
 * 0F 01 /3		LIDT m16&32		Load m into IDTR
 *
 */
BOOL OPCHndlrSysIO_GdtIdMsw(PDASMSTATE pstState, BYTE bOpcode)	// 0f 01 LGDT/SGDT/LIDT/...
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	BYTE bReg;
	static WCHAR awszMnemonics[][MAX_OPCODE_NAME_LEN+1] = 
					{L"sgdt", L"sidt", L"lgdt", L"lidt", L"smsw", L"!", L"lmsw", L"invlpg"};

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bReg, NULL);
	ASSERT(bReg != 5 && bReg >= 0 && bReg <= 7);
	StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", awszMnemonics[bReg]);
	switch(bReg)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_48BIT;
			break;

		case 4:
		case 6:
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;
			break;

		case 7:
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_UNDEF;
			break;
		
		default:
			wprintf_s(L"OPCHndlrSysIO_GdtIdMsw(): Invalid opcode extension %u\n", bReg);
			return FALSE;
	}// switch(bReg)

	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*	** CLTS **
 * 0F 06	CLTS	Clears TS flag in CR0 
 */
BOOL OPCHndlrSysIO_CLTS(PDASMSTATE pstState, BYTE bOpcode)		// Clear Task-Switched Flag in CR0
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

BOOL OPCHndlrSysIO_INVD(PDASMSTATE pstState, BYTE bOpcode)		// Invalidate internal caches
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** WBINVD **
 * 0F 09	WBINVD
 */
BOOL OPCHndlrSysIO_WBINVD(PDASMSTATE pstState, BYTE bOpcode)		// write-back and invalidate cache
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** UD2 **
 * 0B	UD2		Raise invalid opcode exception
 */
BOOL OPCHndlrSysIO_UD2(PDASMSTATE pstState, BYTE bOpcode)		// undefined instruction
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** WRMSR **
 * 0F 30	WRMSR
 */
BOOL OPCHndlrSysIO_WRMSR(PDASMSTATE pstState, BYTE bOpcode)		// Write To Model Specific Register
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** RDTSC **
 * 0F 31	RDTSC
 */
BOOL OPCHndlrSysIO_RDTSC(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** RDMSR **
 * 0F 32	RDMSR	Load MSR specified by ECX into EDX:EAX
 */
BOOL OPCHndlrSysIO_RDMSR(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** RDPMC **
 * 0F 33	RDPMC
 */
BOOL OPCHndlrSysIO_RDPMC(PDASMSTATE pstState, BYTE bOpcode)		// Read Performance Monitoring Counters
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** SYSENTER **
 * 0F, 34	SYSENTER	Transition to System Call Entry Point
 */
BOOL OPCHndlrSysIO_SYSENTER(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** SYSEXIT **
 * 0F, 35	SYSEXIT		Transition from System Call Entry Point
 */
BOOL OPCHndlrSysIO_SYSEXIT(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** PREFETCHxx **
 * 
 */
BOOL OPCHndlrSysIO_Prefetch(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	return FALSE;
}

/*	** CPUID **
 * 0F A2	CPUID
 */
BOOL OPCHndlrSysIO_CPUID(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** RSM **
 * 0F AA	RSM		Resume operation of interrupted program
 */
BOOL OPCHndlrSysIO_RSM(PDASMSTATE pstState, BYTE bOpcode)		// Resume from System Mgmt mode
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}


// Opcodes that may mean any one of multiple instructions

/*	
 *	** BT **
 * 0F BA /4 ib	BT r/m16,imm8
 * 0F BA /4 ib	BT r/m32,imm8
 *	** BTC **
 * 0F BA /7 ib	BTC r/m16,imm8
 * 0F BA /7 ib	BTC r/m32,imm8
 *	** BTR **
 * 0F BA /6 ib	BTR r/m16,imm8
 * 0F BA /6 ib	BTR r/m32,imm8
 *	** BTS **
 * 0F BA /5 ib	BTS r/m16,imm8
 * 0F BA /5 ib	BTS r/m32,imm8
 */
BOOL OPCHndlrMulti_BitTestX(PDASMSTATE pstState, BYTE bOpcode)		// 0f ba
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// Check the 'reg/opcode' field of the ModRM byte
	// This is the extension to the primary opcode.
	BYTE bOpcodeEx;

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);

	switch(bOpcodeEx)
	{
		case 4:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", L"bt");
			return OPCHndlrMem_BT(pstState, bOpcode);

		case 5:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", L"bts");
			return OPCHndlrMem_BTS(pstState, bOpcode);

		case 6:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", L"btr");
			return OPCHndlrMem_BTR(pstState, bOpcode);

		case 7:
			StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"%s", L"btc");
			return OPCHndlrMem_BTC(pstState, bOpcode);

		default:
			wprintf_s(L"OPCHndlrMulti_BitTestX(): Invalid opcode extension %u\n", bOpcodeEx);
			return FALSE;
	}

	return TRUE;	// unreachable I know
}


/*******************
 * MMX instructions
 *******************/

/* ** PADD, PSUB, PMUL, PSAD **
 * All arithmetic instructions operate on 64bit MMX operands.
 * Dest is always an MMX register. Source is always either an
 * MMX register or a quad-word operand in memory. All of them
 * have a /r ModRM byte following the opcode.
 */
BOOL OPCHndlrMMX_PArith(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeSrc = pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	
 * 64h, 65h, 66h: PCMPGT{B,W,D}
 * 74h, 75h, 76h: PCMPEQ{B,W,D}
 * Same as arithmetic instructions.
 */
BOOL OPCHndlrMMX_PCmp(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeSrc = pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** Conversion **
 * PUNPCKLBW, PUNPCKLDW, PUNPCKLDQ are special instructions(opsize).
 * 
 * 0F 60 /r PUNPCKLBW mm,mm/m32		Interleave low-order bytes from mm and mm/m64 into mm.
 * 0F 61 /r PUNPCKLWD mm,mm/m32		Interleave low-order words from mm and mm/m64 into mm.
 * 0F 62 /r PUNPCKLDQ mm,mm/m32		Interleave low-order doublewords from mm and mm/m64 into mm.
 *
 * 0F 63 /r PACKSSWB mm,mm/m64
 * 0F 6B /r PACKSSDW mm,mm/m64
 *
 * 0F 67 /r PACKUSWB mm,mm/m64
 *
 * 0F 68 /r PUNPCKHBW mm,mm/m64		Interleave high-order bytes from mm and mm/m64 into mm.
 * 0F 69 /r PUNPCKHWD mm,mm/m64		Interleave high-order words from mm and mm/m64 intomm.
 * 0F 6A /r PUNPCKHDQ mm,mm/m64		Interleave high-order doublewords from mm and mm/m64 into mm.
 */
BOOL OPCHndlrMMX_PConv(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	// src opsize is 32bits only for PUNPCKL{BW,WD,DQ}
	if(bOpcode == 0x60 || bOpcode == 0x61 || bOpcode == 0x62)
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
	else
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
	pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/* ** P{AND,ANDN,OR,XOR} **
 * Same as arithmetic instructions.
 */
BOOL OPCHndlrMMX_PLogical(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeSrc = pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/* ** **
 * 0F F1 /r		PSLLW mm, mm/m64	Shift words in mm left by amount specified in mm/m64, while shifting in zeroes.
 * 0F F2 /r		PSLLD mm, mm/m64	Shift doublewords in mm left by amount specified in mm/m64, while shifting in zeroes.
 * 0F F3 /r		PSLLQ mm, mm/m64	Shift mm left by amount specified in mm/m64, while shifting in zeroes.
 *
 * 0F 71 /6 ib	PSLLW mm, imm8		Shift words in mm left by imm8, while shifting in zeroes.
 * 0F 72 /6 ib	PSLLD mm, imm8		Shift doublewords in mm by imm8, while shifting in zeroes.
 * 0F 73 /6 ib	PSLLQ mm, imm8		Shift mm left by Imm8, while shifting in zeroes.
 *
 * 0F D1 /r		PSRLW mm, mm/m64	Shift words in mm right by amount specified in mm/m64 while shifting in zeroes.
 * 0F D2 /r		PSRLD mm, mm/m64	Shift doublewords in mm right by amount specified in mm/m64 while shifting in zeroes.
 * 0F D3 /r		PSRLQ mm, mm/m64	Shift mm right by amount specified in mm/m64 while shifting in zeroes.

 * 0F 71 /2 ib	PSRLW mm, imm8		Shift words in mm right by imm8.
 * 0F 72 /2 ib	PSRLD mm, imm8		Shift doublewords in mm right by imm8.
 * 0F 73 /2 ib	PSRLQ mm, imm8		Shift mm right by imm8 while shifting in zeroes.
 *
 * 0F E1 /r		PSRAW mm,mm/m64		Shift words in mm right by amount specified in mm/m64 while shifting in sign bits.
 * 0F E2 /r		PSRAD mm,mm/m64		Shift doublewords in mm right by amount specified in mm/m64 while shifting in sign bits.
 *
 * 0F 71 /4 ib PSRAW mm, imm8		Shift words in mm right by imm8 while shifting in sign bits 
 * 0F 72 /4 ib PSRAD mm, imm8		Shift doublewords in mm right by imm8 while shifting in sign bits.
 */
BOOL OPCHndlrMMX_PShift(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// only 71h,72h,73h opcodes have an immediate operand

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	if(bOpcode == 0x71 || bOpcode == 0x72 || bOpcode == 0x73)
	{
		pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
	}
	else
		pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	
	pstState->insCurIns.bOperandSizeSrc = pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/* 0F {71,72,73} Shift instructions
 *		/2		/4		/6
 *	71	psrlw	psraw	psllw
 *	72	psrld	psrad	pslld
 *	73	psrlq			psllq
 */
BOOL OPCHndlrMMX_PMulti7x(PDASMSTATE pstState, BYTE bOpcode)	// 
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	static WCHAR awszMnemonics[][MAX_OPCODE_NAME_LEN+1] = {
				L"psrlw", L"psraw", L"psllw",
				L"psrld", L"psrad", L"pslld",
				L"psrlq", L"!",     L"psllq" };

	BYTE bOpcodeEx;
	INT iIndex;

	// get the 'reg' field from the ModRM byte
	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);

	ASSERT(bOpcodeEx == 2 ||bOpcodeEx == 4 || bOpcodeEx == 6);	// assert always?

	bOpcodeEx = bOpcodeEx/2 - 1;	// get zero-based index
	iIndex = (bOpcode - 0x71) * 3 + bOpcodeEx;
	StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszMnemonics[iIndex]);

	return OPCHndlrMMX_PShift(pstState, bOpcode);
}

/*	** MOVx **
 * 0F 6E /r	MOVD mm, r/m32		Move doubleword from r/m32 to mm.
 * 0F 7E /r	MOVD r/m32, mm		Move doubleword from mm to r/m32.
 *
 * 0F 6F /r MOVQ mm, mm/m64		Move quadword from mm/m64 to mm.
 * 0F 7F /r MOVQ mm/m64, mm		Move quadword from mm to mm/m64.
 */
BOOL OPCHndlrMMX_PMov(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	// d-bit must be 1 for load and 0 for store instructions

	//pstState->insCurIns.bDBit = 1;	// always

	switch(pstState->bOpcodeHigh)
	{
		case 0x6:	// des = mm; src = r/m32 OR mm/m64	LOAD
		{
			pstState->insCurIns.bDBit = 1;
			pstState->insCurIns.bRegTypeDest = REGTYPE_MMX;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;
			if(pstState->bOpcodeLow == 0xE)
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;	// used by MODRM only if mem-loc is des
			else
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;	// used by MODRM only if mem-loc is des
				pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;	// only if des is a reg
			}
			break;
		}

		case 0x7:	// src = mm; des = r/m32 OR mm/m64	STORE
		{
			pstState->insCurIns.bDBit = 0;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
			if(pstState->bOpcodeLow == 0xE)
				pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;	// used by MODRM only if mem-loc is src
			else
			{
				pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;	// used by MODRM only if mem-loc is src
				pstState->insCurIns.bRegTypeDest = REGTYPE_MMX;	// only if des is a reg
			}
			break;
		}
	}// switch(pstState->bOpcodeHigh)

	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

BOOL OPCHndlrMMX_EMMS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

/*	** PSHUFW **
 * 0F 70 /r ib PSHUFW mm1, mm2/m64, imm8 
 * Shuffle the words in MM2/Mem based on the encoding in imm8 and store in MM1.
 */
BOOL OPCHndlrMMX_PSHUFW(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fImm = TRUE;
	pstState->insCurIns.bImmType = IMM_8BIT;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeSrc = pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** PINSRW **
 * 0F C4 /r ib PINSRW mm,r32/m16,imm8
 * Insert the word from the lower half of r32 or from Mem16 into the
 * position in MM pointed to by imm8 without touching the other words.
 */
BOOL OPCHndlrMMX_PINSRW(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fImm = TRUE;
	pstState->insCurIns.bImmType = IMM_8BIT;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_MMX64;	// doesn't matter since it is a MMX register
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_16BIT;	// m16
	pstState->insCurIns.bRegTypeDest = REGTYPE_MMX;
	pstState->insCurIns.bRegTypeSrc = REGTYPE_GREG;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** PEXTRW **
 * 0F C5 /r ib PEXTRW r32, mm, imm8 
 * Extract the word pointed to by imm8 from MM and move it to a 32-bit integer register.
 */
BOOL OPCHndlrMMX_PEXTRW(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fImm = TRUE;
	pstState->insCurIns.bImmType = IMM_8BIT;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
	pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;
	pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** PMOVMSKB **
 * 0F D7 /r		PMOVMSKB r32, mm	Move the byte mask of MM to r32.
 */
BOOL OPCHndlrMMX_PMOVMSKB(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
	/*
	 * No need to set src opsize because it is always a MMX register.
	 */
	pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;
	pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}

/*	** **
 * Same as arithmetic instructions.
 */
BOOL OPCHndlrMMX_PMaxMinAvg(PDASMSTATE pstState, BYTE bOpcode)	// PMINUB/PMAXUB/PMINSW/PMAXSW/PAVGB/PVGW
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
	/*
	 * No need to set des opsize because it is always a MMX register.
	 * Src maybe a MMX reg/m64 mem-loc.
	 */
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


/*******************
 * SSE instructions
 *******************/
/*
 * 66 0F 5E /r	DIVPD xmm1, xmm2/m128
 */
BOOL OPCHndlrSSE_Arith(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;

	switch(bOpcode)
	{
		case 0x58:	// add
		{
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"addss");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"addsd");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"addpd");
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			break;
		}

		case 0x59:	// mul
		{
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"mulss");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"mulsd");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"mulpd");
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			break;
		}

		case 0x5C:	// sub
		{
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"subss");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"subsd");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"subpd");
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			break;
		}

		case 0x5E:	// div
		{
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"divss");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"divsd");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"divpd");
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			break;
		}

		default:
			wprintf_s(L"OPCHndlrSSE_Arith(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
	}// switch(bOpcode)


	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;

	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;
}


/*	** UCOMISS/COMISS/CMPPS/CMPSS **
 * 0F C2	/r ib		CMPPS xmm1,xmm2/m128,imm8
 * F3 0F C2 /r ib		CMPSS xmm1,xmm2/m32,imm8
 * F2 0F C2 /r ib		CMPSD xmm1,xmm2/m64, imm8
 * 66 0F C2 /r ib		CMPPD xmm1,xmm2/m128, imm8
 *
 *		Pseudo-Op			Implementation
 * CMPEQSS		xmm1, xmm2	CMPSS xmm1,xmm2, 0
 * CMPLTSS		xmm1, xmm2	CMPSS xmm1,xmm2, 1
 * CMPLESS		xmm1, xmm2	CMPSS xmm1,xmm2, 2
 * CMPUNORDSS	xmm1, xmm2	CMPSS xmm1,xmm2, 3
 * CMPNEQSS		xmm1, xmm2	CMPSS xmm1,xmm2, 4
 * CMPNLTSS		xmm1, xmm2	CMPSS xmm1,xmm2, 5
 * CMPNLESS		xmm1, xmm2	CMPSS xmm1,xmm2, 6
 * CMPORDSS		xmm1, xmm2	CMPSS xmm1,xmm2, 7
 * 
 * 0F 2F	/r COMISS xmm1,xmm2/m32
 * 66 0F 2F /r COMISD xmm1, xmm2/m64
 * 0F 2E	/r UCOMISS xmm1,xmm2/m32
 * 66 0F 2E /r UCOMISD xmm1, xmm2/m64
 */
BOOL OPCHndlrSSE_Cmp(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;

	switch(bOpcode)
	{
		case 0x2E:	// UCOMISS/UCOMISD
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"ucomisd");
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			break;

		case 0x2F:	// COMISS/COMISD
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"comisd");
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			break;

		case 0xC2:	// CMPPS or CMPSS
		{
			pstState->insCurIns.fImm = TRUE;
			pstState->insCurIns.bImmType = IMM_8BIT;

			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				// CMPSS
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			}
			else
			{
				// CMPPS or CMPPD
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			}
			break;
		}// case 0xC2

		default:
		{
			wprintf_s(L"OPCHndlrSSE_Cmp(): Invalid opcode %xh\n", bOpcode);
			return FALSE;
		}

	}// switch(bOpcode)
	
	// set the callback function pointer
	pstState->insCurIns.fpvCallback = vCALLBACK_SSECmp;
	
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*
 * To set the instruction mnemonic once IMM state has
 * read the value of the immediate byte.
 */

void vCALLBACK_SSECmp(PDASMSTATE pstState)
{
	static WCHAR awszCMPPSMnemonics[][MAX_OPCODE_NAME_LEN+1] = 
					{ L"cmpeqps", L"cmpltps", L"cmpleps", L"cmpunordps",
						L"cmpneqps", L"cmpnltps", L"cmpbleps", L"cmpordps" };

	static WCHAR awszCMPSSMnemonics[][MAX_OPCODE_NAME_LEN+1] = 
					{ L"cmpeqss", L"cmpltss", L"cmpless", L"cmpunordss",
						L"cmpneqss", L"cmpnltss", L"cmpnless", L"cmpordss" };

	static WCHAR awszCMPPDMnemonics[][MAX_OPCODE_NAME_LEN+1] = 
					{ L"cmpeqpd", L"cmpltpd", L"cmplepd", L"cmpunordpd",
						L"cmpneqpd", L"cmpnltpd", L"cmpnlepd", L"cmpordpd" };

	static WCHAR awszCMPSDMnemonics[][MAX_OPCODE_NAME_LEN+1] = 
					{ L"cmpeqsd", L"cmpltsd", L"cmplesd", L"cmpunordsd",
						L"cmpneqsd", L"cmpnltsd", L"cmpnlesd", L"cmpordsd" };

	if(pstState->insCurIns.iImm < 0 || pstState->insCurIns.iImm > 7)
	{
		wprintf_s(L"vCALLBACK_SSECmp(): Invalid imm value for CMPSS/CMPPS: %d\n", pstState->insCurIns.iImm);
		return;
	}

	// Now that we have a valid imm value, set the proper instruction mnemonic
	if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
	{
		// CMPSS
		StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszCMPSSMnemonics[pstState->insCurIns.iImm]);
	}
	else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
	{
		// CMPSD
		StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszCMPSDMnemonics[pstState->insCurIns.iImm]);
	}
	else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
	{
		// CMPPD
		StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszCMPPDMnemonics[pstState->insCurIns.iImm]);
	}
	else
	{
		// CMPPS
		StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszCMPPSMnemonics[pstState->insCurIns.iImm]);
	}

	// Clear fImm so that the immediate is not printed by the DUMP state
	pstState->insCurIns.fImm = FALSE;

	return;
}

/*
 * 0F 15	/r		UNPCKHPS xmm1,xmm2/m128
 * 0F 14	/r		UNPCKLPS xmm1,xmm2/m128
 *
 * 0F 2A	/r		CVTPI2PS xmm, mm/m64
 * F3 0F 2A /r		CVTSI2SS xmm, r/m32
 * 66 0F 2A /r		CVTPI2PD xmm, mm/m64
 *
 * 0F 2C	/r		CVTTPS2PI mm, xmm/m64
 * F3 0F 2C /r		CVTTSS2SI r32, xmm/m32
 *
 * 0F 2D	/r		CVTPS2PI mm, xmm/m64
 * 66 0F 2D /r		CVTPD2PI mm, xmm/m128
 * F2 0F 2D /r		CVTSD2SI r32, xmm/m64
 *
 * 0F 5A	/r		CVTPS2PD xmm1, xmm2/m64
 * 66 0F 5A /r		CVTPD2PS xmm1, xmm2/m128
 * F2 0F 5A /r		CVTSD2SS xmm1, xmm2/m64
 *
 * 0F 5B	/r		CVTDQ2PS xmm1, xmm2/m128
 * 66 0F 5B /r		CVTPS2DQ xmm1, xmm2/m128
 *
 * F3 0F E6	/r		CVTDQ2PD xmm1, xmm2/m64		; dword ints to fp
 * F2 0F E6	/r		CVTPD2DQ xmm1, xmm2/m128	; fp to dword ints
 * 66 0F E6	/r		CVTTPD2DQ xmm1, xmm2/m128	; with truncation
 */
BOOL OPCHndlrSSE_Conv(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	
	switch(bOpcode)
	{
		case 0x14:
		case 0x15:
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			break;

		case 0x2A:
			pstState->insCurIns.bRegTypeDest = REGTYPE_XMM;
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtsi2ss");
				pstState->insCurIns.bRegTypeSrc = REGTYPE_GREG;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtpi2pd");
				pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			}
			else
			{
				pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			}
			break;

		case 0x2C:
		{
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				// CVTTSS2SI
				pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;
				pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
				pstState->insCurIns.bOperandSizeSrc = pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvttss2si");
			}
			else
			{
				pstState->insCurIns.bRegTypeDest = REGTYPE_MMX;
				pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			}
			break;
		}

		case 0x2D:
		{
			pstState->insCurIns.bRegTypeDest = REGTYPE_MMX;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtpd2pi");
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				// F2 0F 2D /r		CVTSD2SI r32, xmm/m64
				pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
				pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtsd2si");
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			break;
		}

		case 0x5A:	// 66 0F 5A /r		CVTPD2PS xmm1, xmm2/m128
					// F2 0F 5A /r		CVTSD2SS xmm1, xmm2/m64
		{
			pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtsd2ss");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtpd2ps");
			}
			else	// 0F 5A	/r		CVTPS2PD xmm1, xmm2/m64
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
			break;
		}

		case 0x5B:
		{
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtps2dq");
			break;
		}

		case 0xE6:
		{
			pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF2)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtpd2dq");
			}
			else if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_MMX64;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"cvtdq2pd");
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			break;
		}

	}// switch(bOpcode)

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*	
 * SSE:
 * ANDPS/ANDNPS/ORPS/XORPS
 * MINPS/MINSS/MAXPS/MAXSS
 *
 * SSE2:
 * ANDNPD
 */
BOOL OPCHndlrSSE_Logical(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
	if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
	else
		pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;

	switch(bOpcode)
	{
		case 0x55: 
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_OPSIZE] == OPC_PREFIX_OPSIZE)
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"andnpd");
			break;
	}

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*	** DataTransfer **
 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 ; Some SSE data transfer instructions like MOVAPS have duplicate 
 ; opcodes for load and store operations. The d-bit matters in 
 ; these cases. It controls how the source and destination operands 
 ; are specified in the ModRM byte.
 ; Load is memory -> register and Store is register -> memory. 
 ; The only way to specify a memory location using the ModRM byte 
 ; is via the Mod+RM bits. So, a load instruction will always have 
 ; operands specified like Mod+RM -> Reg, i.e., des = 'reg' bits 
 ; and src = 'mod+rm' bits and a store instruction will always have 
 ; Reg -> Mod+RM, i.e., des = 'mod+rm' bits and src = 'reg' bits.
 ; So a SSE load instruction must be processed as if the d-bit is 1
 ; and a SSE store instruction must be processed as if the d-bit is 
 ; 0.
 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 */
BOOL OPCHndlrSSE_Mov(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;

	// dbit may be of significance too: load->d=1, store->d=0
	// regtype may be MMX/XMM/GREG
	// size may be 32/64/128

	switch(bOpcode)
	{
		case 0x10:	// load:xmm/m 128/32 MOVUPS, MOVSS
		{
			pstState->insCurIns.bDBit = 1;
			pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"movss");
			}
			else
				pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			break;
		}

		case 0x11:	// store:xmm/m 128/32 MOVUPS, MOVSS
		{
			pstState->insCurIns.bDBit = 0;
			pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			if(pstState->insCurIns.abPrefixes[PREFIX_INDEX_LOCKREP] == OPC_PREFIX_SIMDF3)
			{
				pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"movss");
			}
			else
				pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_SSE128;
			break;
		}

		case 0x12:	// load:m64 MOVLPS, MOVHLPS
		{
			pstState->insCurIns.bDBit = 1;
			pstState->insCurIns.bRegTypeDest = REGTYPE_XMM;

			// if ModRM byte specifies both register operands, it is a MOVHLPS instruction
			if(*pstState->pbCurrentCodePtr >= 0xC0 && *pstState->pbCurrentCodePtr <= 0xFF)
			{
				pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"movhlps");
			}
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
			break;
		}

		case 0x13:	// store:m64 MOVLPS
		{
			pstState->insCurIns.bDBit = 0;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_64BIT;
			break;
		}

		case 0x16:	// load:m64 MOVHPS, MOVLHPS
		{
			pstState->insCurIns.bDBit = 1;
			pstState->insCurIns.bRegTypeDest = REGTYPE_XMM;

			// if ModRM byte specifies both register operands, it is a MOVLHPS instruction
			if(*pstState->pbCurrentCodePtr >= 0xC0 && *pstState->pbCurrentCodePtr <= 0xFF)
			{
				pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
				StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"movlhps");
			}
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_64BIT;
			break;
		}

		case 0x17:	// store:m64 MOVHPS
		{
			pstState->insCurIns.bDBit = 0;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_64BIT;
			break;
		}

		case 0x28:	// load:xmm/m128 MOVAPS
		{
			pstState->insCurIns.bDBit = 1;
			pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
			break;
		}

		case 0x29:	// store:xmm/m128 MOVAPS
		{
			pstState->insCurIns.bDBit = 0;
			pstState->insCurIns.bRegTypeSrc = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_SSE128;
			break;
		}

		case 0x2B:	// store:m128 MOVNTPS
		{
			pstState->insCurIns.bDBit = 0;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_SSE128;
			break;
		}

		case 0x50:	// r32<-xmm MOVMSKPS d=1 verified
		{
			pstState->insCurIns.bDBit = 1;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
			pstState->insCurIns.bRegTypeDest = REGTYPE_GREG;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_32BIT;	// required for GPR
			break;
		}

		case 0xE7:	// m64<-mm MOVNTQ d=0
		{
			pstState->insCurIns.bDBit = 0;
			pstState->insCurIns.bRegTypeSrc = REGTYPE_MMX;
			pstState->insCurIns.bOperandSizeDes = OPERANDSIZE_64BIT;
			break;
		}

		case 0xF7:	// mm<-mm MASKMOVQ d=1
		{
			pstState->insCurIns.bDBit = 1;
			pstState->insCurIns.bRegTypeSrc = pstState->insCurIns.bRegTypeDest = REGTYPE_MMX;
			break;
		}

		default:
			wprintf_s(L"OPCHndlrSSE_Mov(): Invalid opcode %xh\n", bOpcode);
			return FALSE;

	}// switch(bOpcode)

	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*	** SHUFPS **
 * 0F C6 /r ib	SHUFPS xmm1,xmm2/m128,imm8		Shuffle Single.
 */
BOOL OPCHndlrSSE_SHUFPS(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;

	pstState->insCurIns.fImm = TRUE;
	pstState->insCurIns.bImmType = IMM_8BIT;

	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*
 * FXSAVE	/0		m512
 * FXSTOR	/1		m512
 * LDMXCSR	/2		m32
 * STMXCSR	/3		m32
 * SFENCE	/7
 */
BOOL OPCHndlrSSE_MultiAE(PDASMSTATE pstState, BYTE bOpcode)	// FXSAVE/FXRSTOR/LDMXCSR/STMXCSR/SFENCE
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	static WCHAR awszAEMnemonics[][MAX_OPCODE_NAME_LEN+1] = {
					L"fxsave", L"fxstor", L"ldmxcsr", L"stmxcsr",
					L"",L"", L"", L"sfence" };

	BYTE bOpcodeEx = 0xff;

	// Get the opcode extension
	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;

	// Not performing bound checks for bOpcodeEx because I have
	// accessed the awszARMnemonics array within the switch cases
	// where the bOpcodeEx has valid values.
	
	switch(bOpcodeEx)
	{
		case 0:
		case 1:
			StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszAEMnemonics[bOpcodeEx]);
			// pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_UNDEF;	// m512 so no ptrstr
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 2:
		case 3:
			StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszAEMnemonics[bOpcodeEx]);
			pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_32BIT;
			pstState->dsNextState = DASM_STATE_MOD_RM;
			break;

		case 7:
			StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszAEMnemonics[bOpcodeEx]);
			pstState->dsNextState = DASM_STATE_DUMP;
			break;

		default:
			wprintf_s(L"OPCHndlrSSE_MultiAE(): Invalid opcode extension %d\n", bOpcodeEx);
			return FALSE;
	}

	return TRUE;
}

/*	** PREFETCH* **
 * 0F 18 /1	PREFETCHT0 m8		Move data specified by address closer to the processor using the t0 hint.
 * 0F 18 /2 PREFETCHT1 m8		Move data specified by address closer to the processor using the t1 hint.
 * 0F 18 /3 PREFETCHT2 m8		Move data specified by address closer to the processor using the t2 hint.
 * 0F 18 /0 PREFETCHTNTA m8		Move data specified by address closer to the processor using the nta hint.
 */
BOOL OPCHndlrSSE_Prefetch18(PDASMSTATE pstState, BYTE bOpcode)	// 0F 18 PREFETCH{T0,T1,T2,NTA}
{
	#ifdef _DEBUG
		wprintf_s(L"%s(): %xh\n", __FUNCTIONW__, bOpcode);
	#endif

	static WCHAR awszPrefMnemonics[][MAX_OPCODE_NAME_LEN+1] = {
								L"prefetchnta", L"prefetcht0", L"prefetcht1", L"prefetcht2" };

	BYTE bOpcodeEx;

	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_DIGIT;

	// pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_UNDEF;	// dumpbin says so

	Util_vSplitModRMByte(*pstState->pbCurrentCodePtr, NULL, &bOpcodeEx, NULL);
	
	// No need to check < 0 because bOpcodeEx is unsigned char
	if(bOpcodeEx > 3)
	{
		wprintf_s(L"OPCHndlrSSE_Prefetch18(): Invalid opcode extension %d\n", bOpcodeEx);
		return FALSE;
	}

	StringCchCopy(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszPrefMnemonics[bOpcodeEx]);
	pstState->dsNextState = DASM_STATE_MOD_RM;

	return TRUE;
}

/*	** AES Instructions **
 * 66 0F 38 DB /r		AESIMC xmm1, xmm2/m128
 * 66 0F 38 DC /r		AESENC xmm1, xmm2/m128
 * 66 0F 38 DD /r		AESENCLAST xmm1, xmm2/m128
 * 66 0F 38 DE /r		AESDEC xmm1, xmm2/m128
 * 66 0F 38 DF /r		AESDECLAST xmm1, xmm2/m128
 * 66 0F 3A DF /r ib	AESKEYGENASSIST xmm1,xmm2/m128, imm8
 */
BOOL OPCHndlrAES(PDASMSTATE pstState, BYTE bOpcodeThirdByte)
{
	#ifdef _DEBUG
		wprintf_s(L"OPCHndlrAES(): %xh\n", bOpcodeThirdByte);
	#endif

	static WCHAR awszAESMnemonics[][MAX_INS_OP_STR_LEN+1] = {
					L"aesimc", L"aesenc", L"aesenclast", L"aesdec", L"aesdeslast" };

	INT iIndex = 0;

	pstState->insCurIns.bDBit = 1;
	pstState->insCurIns.fSpecialInstruction = TRUE;
	pstState->insCurIns.fSSEIns = TRUE;
	pstState->insCurIns.bRegTypeDest = pstState->insCurIns.bRegTypeSrc = REGTYPE_XMM;
	pstState->insCurIns.bOperandSizeSrc = OPERANDSIZE_SSE128;
	pstState->insCurIns.fModRM = TRUE;
	pstState->insCurIns.bModRMType = MODRM_TYPE_R;
	if(bOpcodeThirdByte == 0xDF)
	{
		pstState->insCurIns.fImm = TRUE;
		pstState->insCurIns.bImmType = IMM_8BIT;
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), L"aeskeygenassist");
	}
	else
	{
		iIndex = bOpcodeThirdByte - 0xDB;
		if(iIndex < 0 || iIndex >= _countof(awszAESMnemonics))
		{
			wprintf_s(L"OPCHndlrAES(): Invalid index %d calculated from opcode byte %xh\n", iIndex, bOpcodeThirdByte);
			return FALSE;
		}
		StringCchPrintf(pstState->wszCurInsStr, _countof(pstState->wszCurInsStr), awszAESMnemonics[iIndex]);
	}
	
	pstState->dsNextState = DASM_STATE_MOD_RM;
	return TRUE;

}// OPCHndlrAES()

// Invalid opcodes handler
BOOL OPCHndlr_INVALID(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"OPCHndlr_INVALID(): %xh\n", bOpcode);
	#endif
	pstState->dsNextState = DASM_STATE_DUMP;
	return TRUE;
}

// Instructions I don't know yet
BOOL OPCHndlrUnKwn_SSE(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"OPCHndlrUnKwn_SSE(): %xh\n", bOpcode);
	#endif
	//pstState->dsNextState = DASM_STATE_DUMP;
	return FALSE;
}

BOOL OPCHndlrUnKwn_MMX(PDASMSTATE pstState, BYTE bOpcode)
{
	#ifdef _DEBUG
		wprintf_s(L"OPCHndlrUnKwn_MMX(): %xh\n", bOpcode);
	#endif
	//pstState->dsNextState = DASM_STATE_DUMP;
	return FALSE;
}

BOOL OPCHndlrUnKwn_(PDASMSTATE pstState, BYTE bOpcode)		// call this when you don't know anything about the opcode
{
	#ifdef _DEBUG
		wprintf_s(L"OPCHndlrUnKwn_(): %xh\n", bOpcode);
	#endif
	// pstState->dsNextState = DASM_STATE_DUMP;
	return FALSE;
}
