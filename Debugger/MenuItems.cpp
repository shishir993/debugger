
#include "Inc\MenuItems.h"

static void vOpenProgramProcess(HMENU hMenu, DWORD dwChangeTo);
static void vTargetControl(HMENU hMenu, DWORD dwChangeTo);

// vMiDebuggerInit()
//
void vMiDebuggerInit(HMENU hMenu)
{
    ASSERT(ISVALID_HANDLE(hMenu));

    // Everything is enabled by default when program starts up
    // So disable the target controls since no target is loaded at init
    vTargetControl(hMenu, MF_GRAYED);
    return;
}

// vMiDebugSessionStart()
// 
void vMiDebugSessionStart(HMENU hMenu)
{
    ASSERT(ISVALID_HANDLE(hMenu));

    vTargetControl(hMenu, MF_ENABLED);
    vOpenProgramProcess(hMenu, MF_GRAYED);
    return;
}

// vMiDebugSessionEnd()
//
void vMiDebugSessionEnd(HMENU hMenu)
{
    ASSERT(ISVALID_HANDLE(hMenu));

    vOpenProgramProcess(hMenu, MF_ENABLED);
    vTargetControl(hMenu, MF_GRAYED);
    return;
}


void vOpenProgramProcess(HMENU hMenu, DWORD dwChangeTo)
{
    EnableMenuItem(hMenu, IDM_DEBUGPROGRAM, dwChangeTo);
    EnableMenuItem(hMenu, IDM_DEBUGPROCESS, dwChangeTo);
}

void vTargetControl(HMENU hMenu, DWORD dwChangeTo)
{
    EnableMenuItem(hMenu, IDM_TERMINATETARGET, dwChangeTo);
    EnableMenuItem(hMenu, IDM_DETACHFROMTARGET, dwChangeTo);
    EnableMenuItem(hMenu, IDM_DUMPANDTERMINATETARGET, dwChangeTo);
    
    EnableMenuItem(hMenu, IDM_CONTINUE, dwChangeTo);
    EnableMenuItem(hMenu, IDM_STEPINTO, dwChangeTo);
    EnableMenuItem(hMenu, IDM_STEPOVER, dwChangeTo);
    EnableMenuItem(hMenu, IDM_STEPOUT, dwChangeTo);
    
    EnableMenuItem(hMenu, IDM_BREAKINTOTARGET, dwChangeTo);

    EnableMenuItem(hMenu, IDM_SUSPENDALLTHREADS, dwChangeTo);
    EnableMenuItem(hMenu, IDM_RESUMEALLTHREADS, dwChangeTo);
    EnableMenuItem(hMenu, IDM_SUSPRESUME, dwChangeTo);
}