
// RArray.h
// Resizable array implementation
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      2015/12/05 Initial version
//

#ifndef _RARRAY_H
#define _RARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Defines.h"

// The resizable array object
typedef struct _rarray CHL_RARRAY, *PCHL_RARRAY;
struct _rarray
{
    UINT curSize;               // Size of array currently
    UINT maxSize;               // Upper limit for size growth. 0 = unlimited.
    CHL_VALTYPE vt;             // Value type being held in the array
    CHL_VAL *pValArray;         // Actual array holding the values

    // Function pointers

    HRESULT (*Create)(_Out_ PCHL_RARRAY pra, _In_ CHL_VALTYPE valType, _In_opt_ UINT initSize, _In_opt_ UINT maxSize);
    HRESULT (*Destroy)(_In_ PCHL_RARRAY pra);
    HRESULT (*Read)(_In_ PCHL_RARRAY pra, _In_ UINT index, _Out_opt_ PVOID pValBuf,
        _Inout_opt_ PINT piBufSize, _In_ BOOL fGetPointerOnly);
    HRESULT (*Write)(_In_ PCHL_RARRAY pra, _In_ UINT index, _In_ PCVOID pVal, _In_opt_ int iBufSize);
    HRESULT (*ClearAt)(_In_ PCHL_RARRAY pra, _In_ UINT index);
    HRESULT (*Resize)(_In_ PCHL_RARRAY pra, _In_ UINT newSize);
    UINT    (*Size)(_In_ PCHL_RARRAY pra);
    UINT    (*MaxSize)(_In_ PCHL_RARRAY pra);

};

// Create a resizable array for the specified value type and optional size specifications.
// Params:
//  pra         : Pointer to a CHL_RARRAY object that holds the created resizable array
//  valType     : Type of the values in the array. Values of enum CHL_VALTYPE.
//  initSize    : Optional. Initial size of array. Default is 2.
//  maxSize     : Optional. Maximum size array can grow to. Default is unlimited.
//
DllExpImp HRESULT CHL_DsCreateRA(_Out_ PCHL_RARRAY pra, _In_ CHL_VALTYPE valType, _In_opt_ UINT initSize, _In_opt_ UINT maxSize);

// Destroy a previously created resizable array. This frees all memory occupied by the underlying array.
// Params:
//  pra     : Pointer to a previously created CHL_RARRAY object
//
DllExpImp HRESULT CHL_DsDestroyRA(_In_ PCHL_RARRAY pra);

// Read the value at the specified index and return the value in the specified buffer
// Params:
//  pra             : Pointer to a previously created CHL_RARRAY object
//  index           : Array index at which to perform the read. Must be less than the current size of array.
//  pValBuf         : Optional. Pointer to a buffer to receive the read value.
//  piBufSize       : Optional. Pointer to UINT that specifies the provided buffer size.
//                    If buffer size is insufficient, this argument will contain the required size on return.
//                    Not required if fGetPointerOnly is TRUE.
//  fGetPointerOnly : Retrieve only a pointer to the stored array value.
//
DllExpImp HRESULT CHL_DsReadRA(_In_ PCHL_RARRAY pra, _In_ UINT index, _Out_opt_ PVOID pValBuf,
        _Inout_opt_ PINT piBufSize, _In_ BOOL fGetPointerOnly);

// Write to the specified array index, the specified value
// Params:
//  pra     : Pointer to a previously created CHL_RARRAY object
//  index   : Array index at which to perform the write. Array is automatically resized if the index is
//            greater than the current size of array, up until maxSize is reached (if specified).
//  pVal    : Value to be stored. For primitive types, this is the primitive value casted to a PCVOID.
//  iBufSize : Size of the value in bytes. For null-terminated strings, zero may be passed.
//            Ignored for primitive types.
//
DllExpImp HRESULT CHL_DsWriteRA(_In_ PCHL_RARRAY pra, _In_ UINT index, _In_ PCVOID pVal, _In_opt_ int iBufSize);

// Clear the value stored at the specified index
// Params:
//  pra             : Pointer to a previously created CHL_RARRAY object
//  index           : Array index at which to perform the clear. Must be less than the current size of array.
//
DllExpImp HRESULT CHL_DsClearAtRA(_In_ PCHL_RARRAY pra, _In_ UINT index);

// Force resize of the resizable array to the specified size. New size can be lower or higher than current size.
// Params:
//  pra     : Pointer to a previously created CHL_RARRAY object
//  newSize : Size of the resized array
//
DllExpImp HRESULT CHL_DsResizeRA(_In_ PCHL_RARRAY pra, _In_ UINT newSize);

// Retrieve current size of the resizable array
// Params:
//  pra     : Pointer to a previously created CHL_RARRAY object
//
DllExpImp UINT CHL_DsSizeRA(_In_ PCHL_RARRAY pra);

// Retrieve maximum allowed size of the resizable array. Returns 0 if no limit is set.
// Params:
//  pra     : Pointer to a previously created CHL_RARRAY object
//
DllExpImp UINT CHL_DsMaxSizeRA(_In_ PCHL_RARRAY pra);


#ifdef __cplusplus
}
#endif

#endif // _RARRAY_H
