
// BinarySearchTree.h
// The quintessential data structure in computer science - the binary search tree
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      2016/01/28 Initial version. Create, destroy, insert and traverse.
//

#ifndef _CHL_BINARY_SEARCHTREE_H
#define _CHL_BINARY_SEARCHTREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Defines.h"
#include "MemFunctions.h"

typedef enum _bstIerationType {
    BstIterationType_PreOrder,
    BstIterationType_InOrder,
    BstIterationType_PostOrder
} CHL_BstIterationType;

typedef struct _bstNode {
    CHL_KEY chlKey;
    CHL_VAL chlVal;
    UINT    treeSize;       // #nodes in the subtree rooted at this node

    struct _bstNode* pLeft;
    struct _bstNode* pRight;
    //struct _bstNode* pParent;

} BSTNODE, *PBSTNODE;

// Foward declare the iterator struct
typedef struct _bstIterator CHL_BST_ITERATOR, *PCHL_BST_ITERATOR;

// Struct representing the BST object
typedef struct _bstTree CHL_BSTREE, *PCHL_BSTREE;
struct _bstTree {
    CHL_KEYTYPE keyType;
    CHL_VALTYPE valType;
    BOOL        fValIsInHeap;
    PBSTNODE    pRoot;
    
    CHL_CompareFn fnKeyCompare;

    // Pointers to binary search tree methods

    HRESULT (*Create)
        (
            _Out_ PCHL_BSTREE pbst,
            _In_ CHL_KEYTYPE keyType,
            _In_ CHL_VALTYPE valType,
            _In_ CHL_CompareFn pfnKeyCompare,
            _In_opt_ BOOL fValInHeapMem
        );

    HRESULT (*Destroy)(_In_ PCHL_BSTREE pbst);

    HRESULT (*Insert)
        (
            _In_ PCHL_BSTREE pbst,
            _In_ PCVOID pvkey,
            _In_ int iKeySize,
            _In_ PCVOID pvVal,
            _In_ int iValSize
        );

    HRESULT (*Find)
        (
            _In_ PCHL_BSTREE pbst,
            _In_ PCVOID pvkey,
            _In_ int iKeySize,
            _Inout_opt_ PVOID pvVal,
            _Inout_opt_ PINT pValsize,
            _In_opt_ BOOL fGetPointerOnly
        );

    HRESULT (*FindMax)
        (
            _In_ PCHL_BSTREE pbst,
            _Inout_opt_ PVOID pvKeyOut,
            _Inout_opt_ PINT pKeySizeOut,
            _In_opt_ BOOL fGetPointerOnly
        );

    HRESULT (*FindMin)
        (
            _In_ PCHL_BSTREE pbst,
            _Inout_opt_ PVOID pvKeyOut,
            _Inout_opt_ PINT pKeySizeOut,
            _In_opt_ BOOL fGetPointerOnly
        );

    HRESULT (*FindFloor)
        (
            _In_ PCHL_BSTREE pbst,
            _In_ PCVOID pvKey,
            _In_ int iKeySize,
            _Inout_opt_ PVOID pvKeyOut,
            _Inout_opt_ PINT pKeySizeOut,
            _In_opt_ BOOL fGetPointerOnly
        );

    HRESULT (*FindCeil)
        (
            _In_ PCHL_BSTREE pbst,
            _In_ PCVOID pvKey,
            _In_ int iKeySize,
            _Inout_opt_ PVOID pvKeyOut,
            _Inout_opt_ PINT pKeySizeOut,
            _In_opt_ BOOL fGetPointerOnly
        );

    HRESULT (*InitIterator)
        (
            _Out_ PCHL_BST_ITERATOR pItr,
            _In_ PCHL_BSTREE pbst,
            _In_ CHL_BstIterationType itrType
            );

    HRESULT (*GetNext)
        (
            _In_ PCHL_BST_ITERATOR pItr,
            _Inout_opt_ PCVOID pvKey,
            _Inout_opt_ PINT pKeysize,
            _Inout_opt_ PVOID pvVal,
            _Inout_opt_ PINT pValSize,
            _In_opt_ BOOL fGetPointerOnly
        );

};

// Iterator used to traverse the tree
struct _bstIterator {
    CHL_BstIterationType itType;
    PCHL_BSTREE pbst;
    PBSTNODE pCur;

    // Access methods

    HRESULT(*GetNext)
        (
            _In_ PCHL_BST_ITERATOR pItr,
            _Inout_opt_ PCVOID pvKey,
            _Inout_opt_ PINT pKeysize,
            _Inout_opt_ PVOID pvVal,
            _Inout_opt_ PINT pValSize,
            _In_opt_ BOOL fGetPointerOnly
        );
};


// -------------------------------------------
// Functions exported

// Creates a binary search tree and returns a pointer to CHL_BSTREE object 
// which can be used for later operations on the tree.
// Params:
//      pbst            : Pointer to a CHL_BSTREE object to initialize
//      keyType         : Type of variable that is used as key - refer to definition of CHL_KEYTYPE
//      valType         : Type of value that is stored - refer to definition of CHL_VALTYPE
//      pfnKeyCompare   : Pointer to function of type CHL_CompareFn that can compare two keys
//      fValInHeapMem   : Set this to true if the value (type is CHL_VT_POINTER) is allocated memory on the heap.
//                        This indicates the tree to free it when an entry is removed.
// 
DllExpImp HRESULT CHL_DsCreateBST
(
    _Out_ PCHL_BSTREE pbst,
    _In_ CHL_KEYTYPE keyType,
    _In_ CHL_VALTYPE valType,
    _In_ CHL_CompareFn pfnKeyCompare,
    _In_opt_ BOOL fValInHeapMem
);

// Destroy the tree by removing all key-value pairs from the tree.
// The CHL_BSTREE object itself is also destroyed.
// Params:
//      pbst: Pointer to the BST object returned by CHL_DsCreateBST function.
//
DllExpImp HRESULT CHL_DsDestroyBST(_In_ PCHL_BSTREE pbst);

// Inserts a key,value pair into the tree. If the key already exists, then the value is over-written
// with the new value. If both the key and value already exist, then nothing is changed in the tree.
// Params:
//      pbst    : Pointer to the binary search tree object returned by CHL_DsCreateBST function.
//      pvkey   : Pointer to the key. For primitive types, this is the primitive value casted to a PCVOID.
//      iKeySize: Size of the key in bytes. For null-terminated strings, zero may be passed.
//      pvVal   : Value to be stored. Please see documentation for details.
//      iValSize: Size of the value in bytes. For null-terminated strings, zero may be passed.
//
DllExpImp HRESULT CHL_DsInsertBST
(
    _In_ PCHL_BSTREE pbst,
    _In_ PCVOID pvkey,
    _In_ int iKeySize,
    _In_ PCVOID pvVal,
    _In_ int iValSize
);

// Find the specified key in the binary search tree.
// Params:
//      pbst            : Pointer to the binary search tree object returned by CHL_DsCreateBST function.
//      pvkey           : Pointer to the key. For primitive types, this is the primitive value casted to a PCVOID.
//      iKeySize        : Size of the key in bytes. For null-terminated strings, zero may be passed.
//      pvVal           : Optional. Pointer to a buffer to receive the value, if found.
//      pValsize        : Optional. Size of the value buffer in bytes. If specified size is insufficient, the function
//                        returns the required size back in this parameter.
//      fGetPointerOnly : Applies to value of type CHL_VT_USEROBJECT/CHL_VT_STRING/CHL_VT_WSTRING - 
//                        If this is TRUE, function returns a pointer to the stored value in the buffer pvVal.
//                        Otherwise, the full value is copied into the pvVal buffer.
//
DllExpImp HRESULT CHL_DsFindBST
(
    _In_ PCHL_BSTREE pbst,
    _In_ PCVOID pvkey,
    _In_ int iKeySize,
    _Inout_opt_ PVOID pvVal,
    _Inout_opt_ PINT pValsize,
    _In_opt_ BOOL fGetPointerOnly
);

// Get the maximum key in the binary search tree
// Params:
//      pbst            : Pointer to the binary search tree object returned by CHL_DsCreateBST function.
//      pvKey           : Optional. Pointer to buffer to receive the maximum key.
//      pKeySize        : Optional. Size of the key buffer in bytes. If specified size is insufficient, the function
//                        returns the required size back in this parameter.
//      fGetPointerOnly : Applies to value of type CHL_VT_USEROBJECT/CHL_VT_STRING/CHL_VT_WSTRING - 
//                        If this is TRUE, function returns a pointer to the stored key in the buffer pvKeyOut.
//                        Otherwise, the full value is copied into the pKeySizeOut buffer.
//
DllExpImp HRESULT CHL_DsFindMaxBST
(
    _In_ PCHL_BSTREE pbst,
    _Inout_opt_ PVOID pvKeyOut,
    _Inout_opt_ PINT pKeySizeOut,
    _In_opt_ BOOL fGetPointerOnly
);

// Get the minimum key in the binary search tree
// Params:
//      pbst            : Pointer to the binary search tree object returned by CHL_DsCreateBST function.
//      pvKey           : Optional. Pointer to buffer to receive the minimum key.
//      pKeySize        : Optional. Size of the key buffer in bytes. If specified size is insufficient, the function
//                        returns the required size back in this parameter.
//      fGetPointerOnly : Applies to value of type CHL_VT_USEROBJECT/CHL_VT_STRING/CHL_VT_WSTRING - 
//                        If this is TRUE, function returns a pointer to the stored key in the buffer pvKeyOut.
//                        Otherwise, the full value is copied into the pKeySizeOut buffer.
//
DllExpImp HRESULT CHL_DsFindMinBST
(
    _In_ PCHL_BSTREE pbst,
    _Inout_opt_ PVOID pvKey,
    _Inout_opt_ PINT pKeysize,
    _In_opt_ BOOL fGetPointerOnly
);

// Get the floor of specified key in the binary search tree
// Params:
//      pbst            : Pointer to the binary search tree object returned by CHL_DsCreateBST function.
//      pvKey           : Pointer to the key. For primitive types, this is the primitive value casted to a PCVOID.
//      iKeySize        : Size of the key in bytes. For null-terminated strings, zero may be passed.
//                        returns the required size back in this parameter.
//      pvKeyOut        : Optional. Pointer to buffer to receive the floor key, if found.
//      pKeySizeOut     : Optional. Size of the key buffer in bytes. If specified size is insufficient, the function
//                        returns the required size back in this parameter.
//      fGetPointerOnly : Applies to value of type CHL_VT_USEROBJECT/CHL_VT_STRING/CHL_VT_WSTRING - 
//                        If this is TRUE, function returns a pointer to the stored key in the buffer pvKeyOut.
//                        Otherwise, the full value is copied into the pKeySizeOut buffer.
//
DllExpImp HRESULT CHL_DsFindFloorBST
(
    _In_ PCHL_BSTREE pbst,
    _In_ PCVOID pvKey,
    _In_ int iKeySize,
    _Inout_opt_ PVOID pvKeyOut,
    _Inout_opt_ PINT pKeySizeOut,
    _In_opt_ BOOL fGetPointerOnly
);

// Get the ceil of specified key in the binary search tree
// Params:
//      pbst            : Pointer to the binary search tree object returned by CHL_DsCreateBST function.
//      pvKey           : Pointer to the key. For primitive types, this is the primitive value casted to a PCVOID.
//      iKeySize        : Size of the key in bytes. For null-terminated strings, zero may be passed.
//                        returns the required size back in this parameter.
//      pvKeyOut        : Optional. Pointer to buffer to receive the ceil key, if found.
//      pKeySizeOut     : Optional. Size of the key buffer in bytes. If specified size is insufficient, the function
//                        returns the required size back in this parameter.
//      fGetPointerOnly : Applies to value of type CHL_VT_USEROBJECT/CHL_VT_STRING/CHL_VT_WSTRING - 
//                        If this is TRUE, function returns a pointer to the stored key in the buffer pvKeyOut.
//                        Otherwise, the full value is copied into the pKeySizeOut buffer.
//
DllExpImp HRESULT CHL_DsFindCeilBST
(
    _In_ PCHL_BSTREE pbst,
    _In_ PCVOID pvKey,
    _In_ int iKeySize,
    _Inout_opt_ PVOID pvKeyOut,
    _Inout_opt_ PINT pKeySizeOut,
    _In_opt_ BOOL fGetPointerOnly
);

// Initialize the iterator object for use with the specified BST.
// Params:
//      pItr    : Pointer to the iterator object to initialize.
//      pbst    : Pointer to the binary search tree object returned by CHL_DsCreateBST function.
//      itrType : Type of iteration to perform (PreOrder/InOrder/PostOrder).
//
DllExpImp HRESULT CHL_DsInitIteratorBST
(
    _Out_ PCHL_BST_ITERATOR pItr,
    _In_ PCHL_BSTREE pbst,
    _In_ CHL_BstIterationType itrType
);

// Get the next entry in the tree using the specified iterator object.
// Params:
//      pItr            : The iterator object that was initialized by CHL_DsInitIteratorBST.
//      pvKey           : Optional. Pointer to buffer to receive the key of the next entry.
//      pKeySize        : Optional. Size of the key buffer in bytes. If specified size is insufficient, the function
//                        returns the required size back in this parameter.
//      pvVal           : TODO.
//      pvalsize        : TODO.
//      fGetPointerOnly : TODO.
//
DllExpImp HRESULT CHL_DsGetNextBST
(
    _In_ PCHL_BST_ITERATOR pItr, 
    _Inout_opt_ PVOID pvKey,
    _Inout_opt_ PINT pKeysize,
    _Inout_opt_ PVOID pvVal,
    _Inout_opt_ PINT pValSize,
    _In_opt_ BOOL fGetPointerOnly
);

#ifdef __cplusplus
}
#endif

#endif // _CHL_BINARY_SEARCHTREE_H
