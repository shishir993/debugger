
// Hashtable.h
// Hashtable implementation with buckets
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      Unknown history!
//      09/09/14 Refactor to store defs in individual headers.
//      09/12/14 Naming convention modifications
//

#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"
#include "MemFunctions.h"

// hashtable node
typedef struct _hashTableNode {
    BOOL fOccupied;
    CHL_KEY chlKey;
    CHL_VAL chlVal;
    struct _hashTableNode *pnext;
}HT_NODE;

// Foward declare the iterator struct
struct _hashtableIterator;

// hashtable itself
typedef struct _hashtable CHL_HTABLE, *PCHL_HTABLE;
struct _hashtable {
    CHL_KEYTYPE keyType;    // Type information for the hashtable key
    CHL_VALTYPE valType;    // Type information for the hashtable value
    BOOL fValIsInHeap;      // Whether value was allocated on heap by client (for CHL_VT_POINTER only)
    HT_NODE *phtNodes;      // Pointer to hashtable nodes
    int nTableSize;         // Total number of buckets in the hashtable

    // Access methods
    HRESULT (*Destroy)(PCHL_HTABLE phtable);

    HRESULT (*Insert)(
        PCHL_HTABLE phtable, 
        PCVOID pvkey, 
        int iKeySize, 
        PVOID pvVal, 
        int iValSize);

    HRESULT (*Find)(
        PCHL_HTABLE phtable, 
        PCVOID pvkey, 
        int iKeySize, 
        PVOID pvVal, 
        PINT pvalsize,
        BOOL fGetPointerOnly);

    HRESULT (*Remove)(PCHL_HTABLE phtable, PCVOID pvkey, int iKeySize);

    HRESULT (*InitIterator)(PCHL_HTABLE phtable, struct _hashtableIterator *pItr);
    HRESULT (*GetNext)(
        struct _hashtableIterator *pItr,
        PCVOID pvKey, 
        PINT pkeysize,
        PVOID pvVal, 
        PINT pvalsize,
        BOOL fGetPointerOnly);

    void (*Dump)(PCHL_HTABLE phtable);
};

// Structure that defines the iterator for the hashtable
// Callers can use this to iterate through the hashtable
// and get all (key,value) pairs one-by-one
typedef struct _hashtableIterator {
    int opType;
    int nCurIndex;              // current position in the main bucket
    HT_NODE *phtCurNodeInList;  // current position in the sibling list
    PCHL_HTABLE pMyHashTable;   // Pointer to the hashtable to work on
}CHL_HT_ITERATOR;

// -------------------------------------------
// Functions exported

// Creates a hashtable and returns a pointer which can be used for later operations
// on the table.
// Params:
//      pHTableOut: Address of pointer where to copy the pointer to the hashtable
//      nEstEntries: Estimated number of entries that would be in the table at any given time.
//                   This is used to determine the initial size of the hashtable.
//      keyType: Type of variable that is used as key - a string or a number
//      valType: Type of value that is stored - number, string or void(can be anything)
//      fValInHeapMem: Set this to true if the value(type is CHL_VT_POINTER) is allocated memory on the heap.
//                     This indicates the hash table to free it when a table entry is removed.
// 
DllExpImp HRESULT CHL_DsCreateHT(
    _Inout_ CHL_HTABLE **pHTableOut, 
    _In_ int nEstEntries, 
    _In_ CHL_KEYTYPE keyType, 
    _In_ CHL_VALTYPE valType, 
    _In_opt_ BOOL fValInHeapMem);

// Destroy the hashtable by removing all key-value pairs from the hashtable.
// The CHL_HTABLE object itself is also destroyed.
// Params:
//      phtable: Pointer to the hashtable object returned by CHL_DsCreateHT function.
//
DllExpImp HRESULT CHL_DsDestroyHT(_In_ CHL_HTABLE *phtable);

// Inserts a key,value pair into the hash table. If the key already exists, then the value is over-written
// with the new value. If both the key and value already exist, then nothing is changed in the hash table.
// Params:
//      phtable: Pointer to the hashtable object returned by CHL_DsCreateHT function.
//      pvkey: Pointer to the key. For primitive types, this is the primitive value casted to a PCVOID.
//      iKeySize: Size of the key in bytes. For null-terminated strings, zero may be passed.
//      pvVal: Value to be stored. Please see documentation for details.
//      iValSize: Size of the value in bytes. For null-terminated strings, zero may be passed.
//
DllExpImp HRESULT CHL_DsInsertHT(
    _In_ CHL_HTABLE *phtable, 
    _In_ PCVOID pvkey, 
    _In_ int iKeySize, 
    _In_ PCVOID pvVal, 
    _In_ int iValSize);

// Find the specified key in the hash table.
// Params:
//      phtable: Pointer to the hashtable object returned by CHL_DsCreateHT function.
//      pvkey: Pointer to the key. For primitive types, this is the primitive value casted to a PCVOID.
//      iKeySize: Size of the key in bytes. For null-terminated strings, zero may be passed.
//      pvVal: Optional. Pointer to a buffer to receive the value, if found.
//      pvalsize: Optional. Size of the value buffer in bytes. If specified size is insufficient, the function
//          returns the required size back in this parameter.
//      fGetPointerOnly: Applies to value of type CHL_VT_USEROBJECT/CHL_VT_STRING/CHL_VT_WSTRING - 
//          If this is TRUE, function returns a pointer to the stored value in the buffer pvVal.
//          Otherwise, the full value is copied into the pvVal buffer.
//
DllExpImp HRESULT CHL_DsFindHT(
    _In_ CHL_HTABLE *phtable, 
    _In_ PCVOID pvkey, 
    _In_ int iKeySize, 
    _Inout_opt_ PVOID pvVal, 
    _Inout_opt_ PINT pvalsize,
    _In_opt_ BOOL fGetPointerOnly);

// Deletes the specified key from the hash table.
// Params:
//      phtable: Pointer to the hashtable object returned by CHL_DsCreateHT function.
//      pvkey: Pointer to the key. For primitive types, this is the primitive value casted to a PCVOID.
//      iKeySize: Size of the key in bytes. For null-terminated strings, zero may be passed.
//
DllExpImp HRESULT CHL_DsRemoveHT(_In_ CHL_HTABLE *phtable, _In_ PCVOID pvkey, _In_ int iKeySize);

// Initialize the iterator object for use with the specified hashtable.
// Params:
//      pItr: Pointer to the iterator object to initialize.
//      phtable: Pointer to the hashtable object returned by CHL_DsCreateHT function.
//
DllExpImp HRESULT CHL_DsInitIteratorHT(_In_ PCHL_HTABLE phtable, _Inout_ CHL_HT_ITERATOR *pItr);

// Get the next element in the hash table using the specified iterator object.
// Params:
//      pItr: The iterator object that was initialized by CHL_DsInitIteratorHT.
//      pvKey: Optional. Pointer to buffer to receive the key of the next item.
//      pKeySize: Optional. Size of the key buffer in bytes. If specified size is insufficient, the function
//          returns the required size back in this parameter.
//      pvVal: Refer documentation of the CHL_DsFindHT() function.
//      pvalsize: Refer documentation of the CHL_DsFindHT() function.
//      fGetPointerOnly: Refer documentation of the CHL_DsFindHT() function.
//
DllExpImp HRESULT CHL_DsGetNextHT(
    _In_ CHL_HT_ITERATOR *pItr, 
    _Inout_opt_ PCVOID pvKey, 
    _Inout_opt_ PINT pkeysize,
    _Inout_opt_ PVOID pvVal, 
    _Inout_opt_ PINT pvalsize,
    _In_opt_ BOOL fGetPointerOnly);

DllExpImp int CHL_DsGetNearestSizeIndexHT(_In_ int maxNumberOfEntries);
DllExpImp void CHL_DsDumpHT(_In_ CHL_HTABLE *phtable);

// Exposing for unit testing
DllExpImp DWORD _GetKeyHash(_In_ PVOID pvKey, _In_ CHL_KEYTYPE keyType, _In_ int iKeySize, _In_ int iTableNodes);

#ifdef __cplusplus
}
#endif

#endif // _HASHTABLE_H
