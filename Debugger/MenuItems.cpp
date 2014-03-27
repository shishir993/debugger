
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

void vMiDebuggerRunning(HMENU hMenu)
{
    EnableMenuItem(hMenu, IDM_TERMINATETARGET, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DETACHFROMTARGET, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DUMPANDTERMINATETARGET, MF_ENABLED);

    EnableMenuItem(hMenu, IDM_BREAKINTOTARGET, MF_GRAYED);

    EnableMenuItem(hMenu, IDM_SUSPENDALLTHREADS, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_RESUMEALLTHREADS, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_SUSPRESUME, MF_GRAYED);

    EnableMenuItem(hMenu, IDM_CONTINUE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_STEPINTO, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_STEPOVER, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_STEPOUT, MF_GRAYED);
}

void vMiDebuggerDebugging(HMENU hMenu)
{
    EnableMenuItem(hMenu, IDM_TERMINATETARGET, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DETACHFROMTARGET, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DUMPANDTERMINATETARGET, MF_GRAYED);

    EnableMenuItem(hMenu, IDM_BREAKINTOTARGET, MF_GRAYED);

    // todo: can we suspend /resume threads when the target has been suspended?
    EnableMenuItem(hMenu, IDM_SUSPENDALLTHREADS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_RESUMEALLTHREADS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_SUSPRESUME, MF_GRAYED);

    EnableMenuItem(hMenu, IDM_CONTINUE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_STEPINTO, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_STEPOVER, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_STEPOUT, MF_GRAYED);
}

static void vOpenProgramProcess(HMENU hMenu, DWORD dwChangeTo)
{
    EnableMenuItem(hMenu, IDM_DEBUGPROGRAM, dwChangeTo);
    EnableMenuItem(hMenu, IDM_DEBUGPROCESS, dwChangeTo);
}

static void vTargetControl(HMENU hMenu, DWORD dwChangeTo)
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