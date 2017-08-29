
// Stack.h
// Stack implementation using resizable array
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      2015/12/08 Initial version
//

#ifndef _CHL_STACK_H
#define _CHL_STACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Defines.h"
#include "RArray.h"

typedef struct _stack CHL_STACK, *PCHL_STACK;
struct _stack
{
    UINT topIndex;
    CHL_RARRAY rarray;

    // Function pointers

    HRESULT (*Create)(_Out_ PCHL_STACK pstk, _In_ CHL_VALTYPE valType, _In_opt_ UINT maxSize);
    HRESULT (*Destroy)(_In_ PCHL_STACK pstk);
    HRESULT (*Push)(_In_ PCHL_STACK pstk, _In_ PCVOID pVal, _In_opt_ int iBufSize);
    HRESULT (*Pop)(_In_ PCHL_STACK pstk, _Out_opt_ PVOID pValBuf, _Inout_opt_ PINT piBufSize);
    HRESULT (*Top)(_In_ PCHL_STACK pstk, _Out_opt_ PVOID pValBuf, _Inout_opt_ PINT piBufSize,
        _In_ BOOL fGetPointerOnly);
    HRESULT (*Peek)(_In_ PCHL_STACK pstk, _In_ UINT index, _Out_opt_ PVOID pValBuf, _Inout_opt_ PINT piBufSize,
        _In_ BOOL fGetPointerOnly);
    UINT (*Size)(_In_ PCHL_STACK pstk);
};

// Create a stack for the specified value type and optional max size specification.
// Params:
//  pstk    : Pointer to a CHL_STACK object that holds the created stack.
//  valType : Type of the values in the array. Values of enum CHL_VALTYPE.
//  maxSize : Optional. Maximum size the stack can grow to. Default is unlimited.
DllExpImp HRESULT CHL_DsCreateSTK(_Out_ PCHL_STACK pstk, _In_ CHL_VALTYPE valType, _In_opt_ UINT maxSize);

// Destroy a previously created stack. This frees all memory occupied by any existing stack elements.
// Params:
//  pstk    : Pointer to a previously created CHL_STACK object
DllExpImp HRESULT CHL_DsDestroySTK(_In_ PCHL_STACK pstk);

// Push specified value on top of the stack.
// Params:
//  pstk    : Pointer to a previously created CHL_STACK object
//  pVal    : Value to be stored. For primitive types, this is the primitive value casted to a PCVOID.
//  iBufSize: Size of the value in bytes. For null-terminated strings, zero may be passed.
//            Ignored for primitive types.
DllExpImp HRESULT CHL_DsPushSTK(_In_ PCHL_STACK pstk, _In_ PCVOID pVal, _In_opt_ int iBufSize);

// Pop the value on top of the stack. 
// Params:
//  pstk            : Pointer to a previously created CHL_STACK object
//  pValBuf         : Optional. Pointer to a buffer to receive the popped value.
//  piBufSize       : Optional. Pointer to UINT that specifies the provided buffer size.
//                    If buffer size is insufficient, this argument will contain the required size on return.
DllExpImp HRESULT CHL_DsPopSTK(_In_ PCHL_STACK pstk, _Out_opt_ PVOID pValBuf, _Inout_opt_ PINT piBufSize);

// Read value at top of stack.
// Params:
//  pstk            : Pointer to a previously created CHL_STACK object
//  pValBuf         : Optional. Pointer to a buffer to receive the read value.
//  piBufSize       : Optional. Pointer to UINT that specifies the provided buffer size.
//                    If buffer size is insufficient, this argument will contain the required size on return.
//                    Not required if fGetPointerOnly is TRUE.
//  fGetPointerOnly : Retrieve only a pointer to the value at top of stack.
//
DllExpImp HRESULT CHL_DsTopSTK(_In_ PCHL_STACK pstk, _Out_opt_ PVOID pValBuf, _Inout_opt_ PINT piBufSize,
    _In_ BOOL fGetPointerOnly);

// Read value at specified index in the stack.
// Params:
//  pstk            : Pointer to a previously created CHL_STACK object
//  index           : Zero-based index. Get the index'th value (top of stack is index 0).
//  pValBuf         : Optional. Pointer to a buffer to receive the read value.
//  piBufSize       : Optional. Pointer to UINT that specifies the provided buffer size.
//                    If buffer size is insufficient, this argument will contain the required size on return.
//                    Not required if fGetPointerOnly is TRUE.
//  fGetPointerOnly : Retrieve only a pointer to the value at top of stack.
DllExpImp HRESULT CHL_DsPeekSTK(_In_ PCHL_STACK pstk, _In_ UINT index, _Out_opt_ PVOID pValBuf, _Inout_opt_ PINT piBufSize,
    _In_ BOOL fGetPointerOnly);

// Get the number of values in the stack currently.
// Params:
//  pstk    : Pointer to a previously created CHL_STACK object
DllExpImp UINT CHL_DsSizeSTK(_In_ PCHL_STACK pstk);

#ifdef __cplusplus
}
#endif

#endif // _CHL_STACK_H
