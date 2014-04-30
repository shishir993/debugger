
#ifndef _DASM_ENGINE_H
#define _DASM_ENGINE_H

#include "DasmInc.h"
#include "Utils.h"

/*
 * defines
 */

#define MAX_OPCODE_NAME_LEN		16		// prefetchnta made me increase this from 10 to 16!
#define MAX_INSRUCTION_LEN		15		// 15 bytes is the max len of one instruction as of now
#define MAX_INS_STRLEN			128
#define MAX_INS_OP_STR_LEN		32		// string length of the mnemonic for operands
#define MAX_PREFIX_STR_LEN		16
#define NUM_CHARS_MNEMONIC		12

// states of the disassembler state machine
#define DASM_STATE_RESET		0
#define DASM_STATE_PREFIX		1
#define DASM_STATE_OPCODE		2
#define DASM_STATE_MOD_RM		3
#define DASM_STATE_SIB			4
#define DASM_STATE_DISP			5
#define DASM_STATE_IMM			6
#define DASM_STATE_DUMP			7
#define DASM_STATE_ERROR		8

// opcode prefixes
//#define OPC_PREFIX_SIMD_EXT		0x0f
#define OPC_PREFIX_SEGOVR_ES	0x26
#define OPC_PREFIX_SEGOVR_CS	0x2e
#define OPC_PREFIX_SEGOVR_SS	0x36
#define OPC_PREFIX_SEGOVR_DS	0x3e
#define OPC_PREFIX_SEGOVR_FS	0x64
#define OPC_PREFIX_SEGOVR_GS	0x65
#define OPC_PREFIX_OPSIZE		0x66
#define OPC_PREFIX_ADSIZE		0x67
#define OPC_PREFIX_LOCK			0xf0
#define OPC_PREFIX_REPNE		0xf2
#define OPC_PREFIX_REPE			0xf3
#define OPC_PREFIX_SIMDF2		0xf2
#define OPC_PREFIX_SIMDF3		0xf3

// opcodes
#define OPC_2BYTE_ESCAPE	0x0f

// ADD
// 00h to 05h *80h to 83h is part of multiple instructions*
#define OPC_ADD_BEG		0x00
#define OPC_ADD_END		0x05

// PUSH
// *ffh is part of multiple instructions*
#define OPC_PUSH_ES		0x06
#define OPC_PUSH_CS		0x0e
#define OPC_PUSH_SS		0x16
#define OPC_PUSH_DS		0x1e
#define OPC_PUSH_BEG	0x50
#define OPC_PUSH_END	0x57
#define OPC_PUSH_AD		0x60
#define OPC_PUSH_68		0x68
#define OPC_PUSH_6A		0x6a
#define OPC_PUSH_FD		0x9c

// POP
// *ffh is part of multiple instructions*
#define OPC_POP_ES	0x07
#define OPC_POP_SS	0x17
#define OPC_POP_DS	0x1f
#define OPC_POP_BEG	0x58
#define OPC_POP_END	0x5f
#define OPC_POP_AD	0x61
#define OPC_POP_8F	0x8f
#define OPC_POP_FD	0x9d

// ENTER
#define OPC_ENTER	0xc8
#define OPC_LEAVE	0xc9

// OR
// *80h to 83h is part of multiple instructions*
#define OPC_OR_BEG	0x08
#define OPC_OR_END	0x0d

// ADC
// *80h to 83h is part of multiple instructions*
#define OPC_ADC_BEG	0x10
#define OPC_ADC_END	0x15

// SBB
// *80h to 83h is part of multiple instructions*
#define OPC_SBB_BEG	0x18
#define OPC_SBB_END	0x1d

// AND
// *80h to 83h is part of multiple instructions*
#define OPC_AND_BEG	0x20
#define OPC_AND_END	0x25

// SUB
//
#define OPC_SUB_BEG	0x28
#define OPC_SUB_END	0x2d

// XOR
//
#define OPC_XOR_BEG	0x30
#define OPC_XOR_END	0x35

// CMP
// 
#define OPC_CMP_BEG	0x38
#define OPC_CMP_END	0x3d
#define OPC_CMP_S1	0xa6
#define OPC_CMP_S2	0xa7

// 'Adjust' opcodes
#define OPC_DAA	0x27
#define OPC_DAS	0x2f
#define OPC_AAA	0x37
#define OPC_AAS	0x3f
#define OPC_AAM	0xd4
#define OPC_AAD	0xd5

// INC
// *ffh is part of multiple instructions*
#define OPC_INC_BEG	0x40
#define OPC_INC_END	0x47

// DEC
// *ffh is part of multiple instructions*
#define OPC_DEC_BEG	0x48
#define OPC_DEC_END	0x4f

#define OPC_BOUND		0x62
#define OPC_ARPL		0x63
#define OPC_IMUL_69		0x69
#define OPC_IMUL_6b		0x6b

// Port IN
#define OPC_IN_S1	0x6c
#define OPC_IN_S2	0x6d
#define OPC_IN_IMM1	0xe4
#define OPC_IN_IMM2	0xe5
#define OPC_IN_DX1	0xec
#define OPC_IN_DX2	0xed

// OUT Port
#define OPC_OUT_S1		0x6e
#define OPC_OUT_S2		0x6f
#define OPC_OUT_IMM1	0xe6
#define OPC_OUT_IMM2	0xe7
#define OPC_OUT_DX1		0xee
#define OPC_OUT_DX2		0xef

// JUMP opcodes
// *ffh is part of multiple instructions*
#define OPC_JMP_BEG		0x70
#define OPC_JMP_END		0x7f
#define OPC_JMP_O		0x70	// overflow
#define OPC_JMP_NO		0x71
#define OPC_JMP_B		0x72	// below
#define OPC_JMP_NB		0x73
#define OPC_JMP_E		0x74	// equal
#define OPC_JMP_NE		0x75
#define OPC_JMP_BE		0x76	// below/equal
#define OPC_JMP_A		0x77	// above
#define OPC_JMP_S		0x78	// signed
#define OPC_JMP_NS		0x79	// not signed
#define OPC_JMP_PE		0x7a	//
#define OPC_JMP_PO		0x7b
#define OPC_JMP_L		0x7c
#define OPC_JMP_GE		0x7d
#define OPC_JMP_LE		0x7e
#define OPC_JMP_G		0x7f
#define OPC_JMP_ECXZ	0xe3
#define OPC_JMP_UNC		0xe9	// unconditional jump
#define OPC_JMP_F		0xea	// ??
#define OPC_JMP_SHORT	0xeb	

// TEST
#define OPC_TEST_84		0x84
#define OPC_TEST_85		0x85
#define OPC_TEST_A8		0xa8
#define OPC_TEST_A9		0xa9

#define OPC_XCHG_86		0x86
#define OPC_XCHG_87		0x87
#define OPC_XCHG_BEG	0x90	// alias for NOP
#define OPC_XCHG_END	0x97

#define OPC_LEA		0x8d

// MOV
#define OPC_MOV_REG_BEG		0x88
#define OPC_MOV_REG_END		0x8b
#define OPC_MOV_SREG_8c		0x8c
#define OPC_MOV_SREG_8e		0x8e
#define OPC_MOV_EAX_BEG		0xa0
#define OPC_MOV_EAX_END		0xa3
#define OPC_MOV_S1			0xa4
#define OPC_MOV_S2			0xa5
#define OPC_MOV_BEG			0xb0
#define OPC_MOV_END			0xbf
#define OPC_MOV_IMM1		0xc6
#define OPC_MOV_IMM2		0xc7

// Load far pointer
// LFS,LGS,LSS are two byte opcodes
#define OPC_LES		0xc4
#define OPC_LDS		0xc5

#define OPC_NOP 0x90	// alias for XCHG eax,eax

// String opcodes
#define OPC_STOS1	0xaa
#define OPC_STOS2	0xab
#define OPC_LODS1	0xac
#define OPC_LODS2	0xad
#define OPC_SCAS1	0xae
#define OPC_SCAS2	0xaf

// defines for array lengths in the _InstSplit structure
#define MAX_PREFIXES	4
#define MAX_OPCODE_LEN	3
#define MAX_DISP_LEN	4	// displacement 8/32 bits
#define MAX_IMM_LEN		4	// immediate operand 8/16/32bits

// prefix types
#define PREFIX_LOCKREP	0x0001	// F0,F2,F3
#define PREFIX_SEG		0x0002
#define PREFIX_OPSIZE	0x0004
#define PREFIX_ADSIZE	0x0008

// disp types
#define DISP_UNDEF	0
#define DISP_8BIT	10
#define DISP_32BIT	11

// Immediate value types
#define IMM_UNDEF	0
#define IMM_8BIT	1
#define IMM_16BIT	2
#define IMM_32BIT	3
#define IMM_48BIT	4	// EA cd JMP ptr16:16 or EA cp JMP ptr16:32

// Operand sizes
#define OPERANDSIZE_UNDEF	0
#define OPERANDSIZE_8BIT	1
#define OPERANDSIZE_16BIT	2
#define OPERANDSIZE_32BIT	3
#define OPERANDSIZE_48BIT	4
#define OPERANDSIZE_64BIT	5
#define OPERANDSIZE_80BIT	6
#define OPERANDSIZE_MMX64	7
#define OPERANDSIZE_SSE128	8

// fSpecialInstruction::speciality!!
#define SPL_INS_TYPE_SIZEHINT	0	// todos
#define SPL_INS_TYPE_REG		1

// ModRM byte type
#define MODRM_TYPE_UNDEF	0
#define MODRM_TYPE_DIGIT	40	// reg is opcode extension
#define MODRM_TYPE_R		41	// use both reg and r/m

// ModRM byte specifies a reg/sreg/special reg as operand
#define REGTYPE_UNDEF		0
#define REGTYPE_GREG		1	// general purpose
#define REGTYPE_SREG		2	// segment
#define REGTYPE_CREG		3	// control
#define REGTYPE_DREG		4	// debug
#define REGTYPE_MMX			5	// mm0-mm7 registers
#define REGTYPE_XMM			6	// xmm0-xmm7

// index into prefixes array
#define PREFIX_INDEX_LOCKREP	0
#define PREFIX_INDEX_SEG		1
#define PREFIX_INDEX_OPSIZE		2
#define PREFIX_INDEX_ADSIZE		3

// Index to register codes array awszRegCodes*
// * 8bit regs *
#define REGCODE_AL		0	
#define REGCODE_CL		1
#define REGCODE_DL		2
#define REGCODE_BL		3
#define REGCODE_AH		4	
#define REGCODE_CH		5
#define REGCODE_DH		6
#define REGCODE_BH		7

// * 16bit regs *
#define REGCODE_AX		0
#define REGCODE_CX		1
#define REGCODE_DX		2
#define REGCODE_BX		3
#define REGCODE_SP		4
#define REGCODE_BP		5
#define REGCODE_SI		6
#define REGCODE_DI		7

// * 32bit regs *
#define REGCODE_EAX		0
#define REGCODE_ECX		1
#define REGCODE_EDX		2
#define REGCODE_EBX		3
#define REGCODE_ESP		4
#define REGCODE_EBP		5
#define REGCODE_ESI		6
#define REGCODE_EDI		7

// * Segment regs *
#define SREGCODE_ES		0
#define SREGCODE_CS		1
#define SREGCODE_SS		2
#define SREGCODE_DS		3
#define SREGCODE_FS		4
#define SREGCODE_GS		5
//#define SREGCODE_RES6	6
//#define SREGCODE_RES7	7

// * Control regs *
#define CREGCODE_CR0		0
//#define CREGCODE_RES1		1
#define CREGCODE_CR2		2
#define CREGCODE_CR3		3
#define CREGCODE_CR4		4
#define CREGCODE_RES5		5
#define CREGCODE_RES6		6
#define CREGCODE_RES7		7

// * Debug regs *
#define DREGCODE_DR0		0
#define DREGCODE_DR1		1
#define DREGCODE_DR2		2
#define DREGCODE_DR3		3
#define DREGCODE_RES4		4
#define DREGCODE_RES5		5
#define DREGCODE_DR6		6
#define DREGCODE_DR7		7

// Operand size strings
#define MAX_PTR_STR			16

#define PTR_STR_INDEX_UNDEF		0	// analogous to OPERANDSIZE_*
#define PTR_STR_INDEX_BYTE		1
#define PTR_STR_INDEX_WORD		2
#define PTR_STR_INDEX_DWORD		3
#define PTR_STR_INDEX_FWORD		4
#define PTR_STR_INDEX_QWORD		5
#define PTR_STR_INDEX_TBYTE		6
#define PTR_STR_INDEX_MMWORD	7
#define PTR_STR_INDEX_XMMWORD	8

#define EFFADDR_SIB		4
#define EFFADDR_DISP32	5

// OPRTYPE_RETVAL values
#define OPRTYPE_ERROR	0
#define OPRTYPE_MEM		1	// specified by GPR/GPR with no displacement
#define OPRTYPE_MEM8	2	// specified by GPR/GPR with disp8
#define OPRTYPE_MEM32	3	// specified by GPR/GPR with disp32
#define OPRTYPE_REG		4	// just a GPR: with Mod = 11
#define OPRTYPE_DISP32	5	// just a disp32
#define OPRTYPE_SIB		6
#define OPRTYPE_SIB8	7	// SIB with disp8
#define OPRTYPE_SIB32	8	// SIB with disp32

/*
 * Macros
 */
#define SET_IMMTYPE_OPSIZE	(	insCurIns.bImmType = \
								insCurIns.wPrefixTypes & PREFIX_OPSIZE ? \
								OPERANDSIZE_16BIT : \
								OPERANDSIZE_32BIT )

/* end of defines */

typedef BYTE	DASM_STATE;
typedef INT		OPRTYPE_RETVAL;		// return type of GetOperandFrom*()

/*
 * structures
 */

typedef struct _DasmState {

    INT structInitialized;

    // struct to hold the various parts of an instruction
    struct _InsSplit {

	    /* 
	     * The boolean values indicate whether the corresponding field 
	     * is used in the instruction or not.
	     */
	    BOOL fPrefix;
	    BOOL fModRM;
	    BOOL fSIB;
	    BOOL fDisp;
	    BOOL fImm;
	    BOOL fImmSignEx;			// sign-extend the imm value??
	    BOOL fCodeOffset;
	    BOOL fDataOffset;			// MOV AL,moffs8 ... ds:[offset]
	    BOOL fSpecialInstruction;	// specimens like ARPL, BOUND, MOVSX!!
	    BOOL fSSEIns;				// Is this an SSE instruction? To handle printing of prefixes.
	    BOOL fWBitAbsent;			// Not all instructions have a w-bit: BSF doesn't

	    /*
	     * length of each field
	     */
	    BYTE nPrefix;	// 0-4 prefixes of 1byte each. Ignore if fPrefix == FALSE.
	    BYTE nDisp;		// 0-4 bytes. Ignore if fDisp == FALSE.
	    BYTE nImm;		// 0-4 bytes. Ignore if fImm == FALSE.

	    /*
	     * types
	     */
	    WORD wPrefixTypes;
	    BYTE bImmType;		// 8/16/32 bits
	    BYTE bDispType;
	    BYTE bModRMType;	// ModRM+Reg / ModRM+OpcodeExtension
	    //BYTE bSplInsType;	// MOV
	    OPRTYPE_RETVAL OprTypeSrc;	// Set by MODRM state and used by DUMP state to
	    OPRTYPE_RETVAL OprTypeDes;	// properly print the instruction string.

	    // Some instructions like ARPL/BOUND have a defined operand size which doesn't
	    // depend on either the 'w' bit or the opsize override prefix.

	    // Also, instructions like MOVSX, MOVZX have different sized source and destination operands.
	
	    // For these instructions, fSpecialInstruction will be TRUE so that MODRM state
	    // can check the operand size and then continue.
	    BYTE bOperandSizeSrc;
	    BYTE bOperandSizeDes;

	    // reg/sreg/splReg
	    // For example: MOV r/m16,Sreg
	    BYTE bRegTypeSrc;
	    BYTE bRegTypeDest;

	    // Boolean to determine whether the source and destination
	    // operand strings(part of the instruction) was already written
	    // in the previous states or not.
	    BOOL fSrcStrSet;
	    BOOL fDesStrSet;
	    BOOL fOpr3StrSet;

	    /*
	     * individual fields
	     */
	    BYTE	bDBit;
	    BYTE	bWBit;
	    BYTE	bModRM;
	    BYTE	bSIB;
	    BYTE	abPrefixes[MAX_PREFIXES];
	    INT		iDisp;	// Max of 32bit disp
	    INT		iImm;	// Max of 32bit imm value

	    // Strings to store the instruction strings to be dumped later in DUMP state
	    WCHAR wszPrefixes[MAX_PREFIX_STR_LEN+1];
	    WCHAR wszCurInsStrSrc[MAX_INS_OP_STR_LEN+1];
	    WCHAR wszCurInsStrDes[MAX_INS_OP_STR_LEN+1];
	    WCHAR wszCurInsStrOpr3[MAX_INS_OP_STR_LEN+1];	// Shift/Rotate instructions with 'cl'/'1' as third operand
	    WCHAR *pwszPtrStr;		// "byte ptr" / "word ptr" / ...
	    WCHAR *pwszEffAddrReg1;	// one of the GP/C/D registers	- source
	    WCHAR *pwszEffAddrReg2;	// one of the GP/C/D registers	- destination

	    // SIB info
	    WCHAR wszScaleIndex[MAX_INS_OP_STR_LEN+1];
	    WCHAR wszBase[MAX_INS_OP_STR_LEN+1];

	    // Function pointer to the CALLBACK function to be called
	    // before jumping to the DUMP state.
	    void (*fpvCallback)(struct _DasmState*);

    }insCurIns;

    PBYTE pbCurrentCodePtr;
    PBYTE pbEndOfCode;

    // No. of bytes in the current instruction; used to dump
    // hex bytes by subtracting this from pbCurrentCodePtr
    INT nBytesCurIns;

    // Variables to store the whole opcode, high and low 4bits of the opcode
    BYTE bFullOpcode;
    BYTE bOpcodeLow;
    BYTE bOpcodeHigh;

    // Used only by FPU instructions
    BYTE bFPUModRM;
    BYTE bFPUReg;

    // Strings that are used to hold the asm instruction after disassembly
    WCHAR wszCurInsStr[MAX_INS_STRLEN+1];
    WCHAR wszCurInsTempStr[MAX_INS_STRLEN+1];

    LONG lDelta;
    DASM_STATE dsNextState;
    BOOL fRunning;

    // When disassembling one instruction at a time, this will store
    // the disassembled instruction when returning back to caller
    WCHAR szDisassembledInst[MAX_INS_STRLEN+1];

}DASMSTATE, *PDASMSTATE;

typedef struct _InsSplit INS_SPLIT;

/* end of structures */

/*
 * function prototypes
 */
BOOL fDisassembler(NCODE_LOCS *pCodeLocs, DWORD dwVirtCodeBase);
BOOL fDoDisassembly(DWORD *pdwCodeSection, DWORD dwSizeOfCodeSection,
					DWORD dwVirtCodeBase);

BOOL fDasmDisassemble(
    PBYTE pbCodeBegin, 
    DWORD dwSizeOfCodeSection,
    INT nInstToDisassemble, 
    BOOL fStopAtFunctionEnd,
    DWORD dwTargetAddressBegin,
    PINT piReturnStatus);

BOOL fDasmDisassembleOne(
    __inout PDASMSTATE pstDasmState,
    __in PBYTE pbCodeBegin, 
    DWORD dwSizeOfCodeSection,
    BOOL fStopAtFunctionEnd,
    DWORD dwTargetAddressBegin,
    __out PINT piReturnStatus);

/* Disasm states */
BOOL fStateReset(PDASMSTATE pstState);
BOOL fStatePrefix(PDASMSTATE pstState);

BOOL fStateOpcode(PDASMSTATE pstState);
BOOL OPCHndlr_2ByteHandler(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlr_3ByteHandler(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_All(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_ModRMRegEx(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_ModRMFullEx(PDASMSTATE pstState, BYTE bOpcode);

BOOL fStateModRM(PDASMSTATE pstState);
BOOL MODRM_fTypeDigit(PDASMSTATE pstState, BYTE bModRM, BYTE bMod, BYTE bRM);
BOOL MODRM_fTypeR(PDASMSTATE pstState, BYTE bModRM, BYTE bMod, BYTE bReg, BYTE bRM);
OPRTYPE_RETVAL MODRM_GetOperandFromModRM(BYTE bMod, BYTE bRM, BYTE bOprSize, BYTE bRegType, 
											__out WCHAR *pwszOprStr, __in DWORD dwOprStrCount);
OPRTYPE_RETVAL MODRM_GetOperandFromReg(BYTE bReg, BYTE bOprSize, BYTE bRegType, 
										__out WCHAR *pwszOprStr, __in DWORD dwOprStrCount);
BOOL MODRM_fSetPtrStr(PDASMSTATE pstState, BYTE bOperandSize, __out WCHAR *pwszPtrStr, DWORD dwPtrStrCount);
BYTE MODRM_bGetOperandSize(PDASMSTATE pstState);

BOOL fStateSIB(PDASMSTATE pstState);
BOOL fStateDisp(PDASMSTATE pstState);
BOOL fStateImm(PDASMSTATE pstState);

BOOL fStateDump(PDASMSTATE pstState);
BOOL fStateDump_ToString(PDASMSTATE pstState);
BOOL fStateDumpOnOpcodeError(PDASMSTATE pstState);
BOOL fStateDumpOnOpcodeError_ToString(PDASMSTATE pstState);
void DUMP_vDumpDataOffset(PDASMSTATE pstState);
void DUMP_vDumpDataOffset_ToString(PDASMSTATE pstState, PDWORD pnTotalCharsWritten);
void DUMP_vGetSegPrefix(BYTE bSegPrefixVal, __out WCHAR **ppwszPrefixStr);

//BOOL fStateDumpToMemory(PWCHAR pszStartOfBuffer, 

// Opcodecode handlers grouped by:
// ALU, Memory, Prefix, Stack, Ctrl_Cond, System_IO

// Stack operations
BOOL OPCHndlrStack_PUSH(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrStack_POP(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrStack_PUSHxx(PDASMSTATE pstState, BYTE bOpcode);	// PUSHAD/PUSHFD
BOOL OPCHndlrStack_POPxx(PDASMSTATE pstState, BYTE bOpcode);		// POPAD/POPFD
BOOL OPCHndlrStack_ENTER(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrStack_LEAVE(PDASMSTATE pstState, BYTE bOpcode);

// Arithmetic instruction handlers
BOOL OPCHndlrALU_ADD(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_SUB(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_MUL(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_DIV(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_INC(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_DEC(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_Shift(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_Shift_SetInsStr(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_SALC(PDASMSTATE pstState, BYTE bOpcode);

// Logical instructions
BOOL OPCHndlrALU_OR(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_AND(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_XOR(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_NOT(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_NEG(PDASMSTATE pstState, BYTE bOpcode);

// Adjust instructions
BOOL OPCHndlrALU_DAA(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_DAS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_AAA(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_AAS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_AAM(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrALU_AAD(PDASMSTATE pstState, BYTE bOpcode);

// Memory operations
BOOL OPCHndlrMem_XCHG(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_MOV(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LEA(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_CWDE(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_CDQ(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_SAHF(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LAHF(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_MOVS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LODS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_STOS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LES(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LDS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_XLAT(PDASMSTATE pstState, BYTE bOpcode);

// Control flow and Conditional instructions
BOOL OPCHndlrCC_CMP(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_BOUND(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_ARPL(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_JUMP(PDASMSTATE pstState, BYTE bOpcode);	// all jumps
BOOL OPCHndlrCC_TEST(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_CMPS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_SCAS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_RETN(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_IRETD(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_CLOOP(PDASMSTATE pstState, BYTE bOpcode);	// conditional loop: LOOPE, ...
BOOL OPCHndlrCC_CALL(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_EFLAGS(PDASMSTATE pstState, BYTE bOpcode);	// EFLAGS manipulators: CMC, CLC, STC, ...

// System and IO instructions
BOOL OPCHndlrSysIO_INS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_OUTS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_WAIT(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_INT(PDASMSTATE pstState, BYTE bOpcode);	//	INT3/INTn/INTO
BOOL OPCHndlrSysIO_IceBP(PDASMSTATE pstState, BYTE bOpcode);	// undocumented INT1
BOOL OPCHndlrSysIO_IN(PDASMSTATE pstState, BYTE bOpcode);	// IN imm
BOOL OPCHndlrSysIO_OUT(PDASMSTATE pstState, BYTE bOpcode);	// OUT imm
BOOL OPCHndlrSysIO_INDX(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_OUTDX(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_HLT(PDASMSTATE pstState, BYTE bOpcode);

// Prefix handlers
BOOL OPCHndlrPrefix_Ovride(PDASMSTATE pstState, BYTE bOpcode);	// override prefixes
BOOL OPCHndlrPrefix_LOCK(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrPrefix_CREP(PDASMSTATE pstState, BYTE bOpcode);	// conditional repetition: REPE/REPNE

// Opcodes that may mean any one of multiple instructions
BOOL OPCHndlrMulti_8x(PDASMSTATE pstState, BYTE bOpcode);		// 0x80 - 0x83
BOOL OPCHndlrMulti_fx(PDASMSTATE pstState, BYTE bOpcode);		// 0xf6 and 0xf7
BOOL OPCHndlrMulti_IncDec(PDASMSTATE pstState, BYTE bOpcode);	// 0xfe: INC/DEC
BOOL OPCHndlrMulti_FF(PDASMSTATE pstState, BYTE bOpcode);		// 0xff

BOOL OPCHndlr_NOP(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlr_HNOP(PDASMSTATE pstState, BYTE bOpcode);

// FPU instructions
//BOOL OPCHndlrFPU_(PDASMSTATE pstState, BYTE bOpcode);

#ifdef UNIT_TESTS_ONLY
void DEngine_FPUUnitTest();
void DEngine_MMXUnitTest();
void DEngine_SSEUnitTest();
#endif

// FPU Basic Arithmetic
BOOL OPCHndlrFPU_FADD(PDASMSTATE pstState, BYTE bOpcode);	// FADDP/FIADD also
BOOL OPCHndlrFPU_FSUB(PDASMSTATE pstState, BYTE bOpcode);	// + FSUBP/FISUB/FSUBR/FSUBRP/FISUBR
BOOL OPCHndlrFPU_FMUL(PDASMSTATE pstState, BYTE bOpcode);	// + FMULP/FIMUL
BOOL OPCHndlrFPU_FDIV(PDASMSTATE pstState, BYTE bOpcode);	// + FDIVR/FIDIV/FDIVP/FDIVRP/FIDIVR
BOOL OPCHndlrFPU_FABS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_FCHS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_FSQRT(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_FPREM(PDASMSTATE pstState, BYTE bOpcode);	// + FPREM1
BOOL OPCHndlrFPU_FRNDINT(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_FXTRACT(PDASMSTATE pstState, BYTE bOpcode);

// FPU Load Constants
BOOL OPCHndlrFPU_Const(PDASMSTATE pstState, BYTE bOpcode);	// all constants

// FPU Data Transfer
BOOL OPCHndlrFPU_FLoad(PDASMSTATE pstState, BYTE bOpcode);		// FLD/FILD/FBLD
BOOL OPCHndlrFPU_FStore(PDASMSTATE pstState, BYTE bOpcode);		// FST/FSTP/FIST/FISTP/FBSTP
BOOL OPCHndlrFPU_FXCH(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_FCMOV(PDASMSTATE pstState, BYTE bOpcode);

// FPU Compare/Classify
BOOL OPCHndlrFPU_FCmpReal(PDASMSTATE pstState, BYTE bOpcode);	// FCOM,P,PP/FUCOM,P,PP/FCOMI,IP
BOOL OPCHndlrFPU_FCmpInts(PDASMSTATE pstState, BYTE bOpcode);	// FICOM,P
BOOL OPCHndlrFPU_FTST(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrFPU_FXAM(PDASMSTATE pstState, BYTE bOpcode);

// FPU Trigonometric
BOOL OPCHndlrFPU_Trig(PDASMSTATE pstState, BYTE bOpcode);	// FSIN/FCOS/FSINCOS/FPTAN/FPATAN

// FPU Log, Exp, Scale
BOOL OPCHndlrFPU_LgExSc(PDASMSTATE pstState, BYTE bOpcode);	// FYL2X/FYL2XP1/F2XM1/FSCALE

// FPU Control
BOOL OPCHndlrFPU_Ctl(PDASMSTATE pstState, BYTE bOpcode);		// No operands
BOOL OPCHndlrFPU_CtlOp(PDASMSTATE pstState, BYTE bOpcode);	// With operands

BOOL OPCHndlrFPU_FNOP(PDASMSTATE pstState, BYTE bOpcode);

BOOL OPCHndlrFPU_Invalid(PDASMSTATE pstState, BYTE bOpcode);


// * Instructions that are 2bytes (first byte is 0x0f) *

// Stack instructions


// Arithmetic instructions
BOOL OPCHndlrALU_XADD(PDASMSTATE pstState, BYTE bOpcode);	// Exchange and Add


// Logical instructions


// Memory
// BOOL OPCHndlrMem_(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LAR(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LSL(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_MOVCrDr(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_BT(PDASMSTATE pstState, BYTE bOpcode);			// Bit Test
BOOL OPCHndlrMem_BTS(PDASMSTATE pstState, BYTE bOpcode);			// Bit Test and Set
BOOL OPCHndlrMem_LSS(PDASMSTATE pstState, BYTE bOpcode);			// Load Full Pointer
BOOL OPCHndlrMem_BTR(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_LFS(PDASMSTATE pstState, BYTE bOpcode);			// Load Full Pointer
BOOL OPCHndlrMem_LGS(PDASMSTATE pstState, BYTE bOpcode);			// Load Full Pointer
BOOL OPCHndlrMem_MOVZX(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_MOVSX(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_BTC(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_BSF(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_BSR(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMem_ByteSWAP(PDASMSTATE pstState, BYTE bOpcode);

// Control flow and Conditional
// BOOL OPCHndlrCC_(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrCC_CMOV(PDASMSTATE pstState, BYTE bOpcode);		// Conditional move
BOOL OPCHndlrCC_SETxx(PDASMSTATE pstState, BYTE bOpcode);	// SET byte on condition
BOOL OPCHndlrCC_CmpXchg(PDASMSTATE pstState, BYTE bOpcode);	// Compare and Exchange


// Sys or IO instructions
// BOOL OPCHndlrSysIO_(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_LdtTrS(PDASMSTATE pstState, BYTE bOpcode);	// 0f 00 LLDT/SLDT/LTR/...
BOOL OPCHndlrSysIO_GdtIdMsw(PDASMSTATE pstState, BYTE bOpcode);	// 0f 01 LGDT/SGDT/LIDT/...
BOOL OPCHndlrSysIO_CLTS(PDASMSTATE pstState, BYTE bOpcode);		// Clear Task-Switched Flag in CR0
BOOL OPCHndlrSysIO_INVD(PDASMSTATE pstState, BYTE bOpcode);		// Invalidate internal caches
BOOL OPCHndlrSysIO_WBINVD(PDASMSTATE pstState, BYTE bOpcode);	// write-back and invalidate cache
BOOL OPCHndlrSysIO_UD2(PDASMSTATE pstState, BYTE bOpcode);		// undefined instruction
BOOL OPCHndlrSysIO_WRMSR(PDASMSTATE pstState, BYTE bOpcode);		// Write To Model Specific Register
BOOL OPCHndlrSysIO_RDTSC(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_RDMSR(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_RDPMC(PDASMSTATE pstState, BYTE bOpcode);		// Read Performance Monitoring Counters
BOOL OPCHndlrSysIO_SYSENTER(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_SYSEXIT(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_CPUID(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSysIO_RSM(PDASMSTATE pstState, BYTE bOpcode);		// Resume from System Mgmt mode
BOOL OPCHndlrSysIO_Prefetch(PDASMSTATE pstState, BYTE bOpcode);	// 0f 18


// Opcodes that may mean any one of multiple instructions
BOOL OPCHndlrMulti_BitTestX(PDASMSTATE pstState, BYTE bOpcode);		// 0f ba

// MMX instructions
//BOOL OPCHndlrMMX_(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PArith(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PCmp(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PConv(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PLogical(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PShift(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PMov(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_EMMS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PSHUFW(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PINSRW(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PEXTRW(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PMOVMSKB(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrMMX_PMaxMinAvg(PDASMSTATE pstState, BYTE bOpcode);	// PMINUB/PMAXUB/PMINSW/PMAXSW/PAVGB/PVGW
BOOL OPCHndlrMMX_PMulti7x(PDASMSTATE pstState, BYTE bOpcode);	// 0F {71,72,73}

// SSE instructions
//BOOL OPCHndlrSSE_(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSSE_Arith(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSSE_Cmp(PDASMSTATE pstState, BYTE bOpcode);
void vCALLBACK_SSECmp(PDASMSTATE pstState);
BOOL OPCHndlrSSE_Conv(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSSE_Logical(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSSE_Mov(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSSE_SHUFPS(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrSSE_MultiAE(PDASMSTATE pstState, BYTE bOpcode);	// FXSAVE/FXRSTOR/LDMXCSR/STMXCSR/SFENCE
BOOL OPCHndlrSSE_Prefetch18(PDASMSTATE pstState, BYTE bOpcode);	// 0F 18 PREFETCH{T0,T1,T2,NTA}

BOOL OPCHndlrAES(PDASMSTATE pstState, BYTE bOpcodeThirdByte);

// Invalid opcodes handler
BOOL OPCHndlr_INVALID(PDASMSTATE pstState, BYTE bOpcode);

// Instructions I don't know yet
BOOL OPCHndlrUnKwn_SSE(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrUnKwn_MMX(PDASMSTATE pstState, BYTE bOpcode);
BOOL OPCHndlrUnKwn_(PDASMSTATE pstState, BYTE bOpcode);		// call this when you don't know anything about the opcode

/* end of function prototypes */

#endif // _DASM_ENGINE_H
