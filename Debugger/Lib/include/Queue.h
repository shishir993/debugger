
// Queue.h
// Contains functions that provide IO operation services
// Shishir Bhat (http://www.shishirbhat.com)
// History
//      Unknown history!
//      09/09/14 Refactor to store defs in individual headers.
//

#ifndef _CHL_QUEUE_H
#define _CHL_QUEUE_H

#ifdef __cplusplus
extern "C" {  
#endif

#include "Defines.h"
#include "LinkedList.h"
#include "MemFunctions.h"

// A Queue object structure
   typedef struct _Queue CHL_QUEUE, *PCHL_QUEUE;
struct _Queue
{
    int nCapacity;  // unused currently
    int nCurItems;
    PCHL_LLIST pList;

    // Access Methods

    HRESULT (*Create)
        (
            _Out_ PCHL_QUEUE *ppQueueObj,
            _In_ CHL_VALTYPE valType,
            _In_opt_ int nEstimatedItems
        );

    HRESULT (*Destroy)
        (
            _In_ PCHL_QUEUE pThis
        );

    HRESULT (*Insert)
        (
            _In_ PCHL_QUEUE pQueueObj,
            _In_ PCVOID pvValue,
            _In_ int nValSize
        );
    
    HRESULT (*Delete)
        (
            _In_ PCHL_QUEUE pQueueObj,
            _Inout_opt_ PVOID pValOut,
            _Inout_opt_ PINT piValBufSize,
            _In_opt_ BOOL fGetPointerOnly
        );
    
    HRESULT (*Peek)
        (
            _In_ PCHL_QUEUE pQueueObj,
            _Inout_opt_ PVOID pValOut,
            _Inout_opt_ PINT piValBufSize,
            _In_opt_ BOOL fGetPointerOnly
        );

    HRESULT(*Find)
    (
        _In_ PCHL_QUEUE pQueueObj,
        _In_ PCVOID pvValue,
        _In_ CHL_CompareFn pfnComparer,
        _In_opt_ PVOID pvValOut,
        _In_opt_ PINT piValBufSize,
        _In_opt_ BOOL fGetPointerOnly
    );
};

// -------------------------------------------
// Functions exported

DllExpImp HRESULT CHL_DsCreateQ
(
    _Out_ PCHL_QUEUE *ppQueueObj,
    _In_ CHL_VALTYPE valType,
    _In_opt_ int nEstimatedItems
);

DllExpImp HRESULT CHL_DsDestroyQ(_In_ PCHL_QUEUE pQueueObj);

DllExpImp HRESULT CHL_DsInsertQ
(
    _In_ PCHL_QUEUE pQueueObj,
    _In_ PCVOID pvValue,
    _In_ int nValSize
);

DllExpImp HRESULT CHL_DsDeleteQ
(
    _In_ PCHL_QUEUE pQueueObj, 
    _Inout_opt_ PVOID pValOut, 
    _Inout_opt_ PINT piValBufSize, 
    _In_opt_ BOOL fGetPointerOnly
);

DllExpImp HRESULT CHL_DsPeekQ
(
    _In_ PCHL_QUEUE pQueueObj,
    _Inout_opt_ PVOID pValOut,
    _Inout_opt_ PINT piValBufSize,
    _In_opt_ BOOL fGetPointerOnly
);

DllExpImp HRESULT CHL_DsFindQ
(
    _In_ PCHL_QUEUE pQueueObj,
    _In_ PCVOID pvValue,
    _In_ CHL_CompareFn pfnComparer,
    _In_opt_ PVOID pvValOut,
    _In_opt_ PINT piValBufSize,
    _In_opt_ BOOL fGetPointerOnly
);

#ifdef __cplusplus
}
#endif

#endif // _CHL_QUEUE_H