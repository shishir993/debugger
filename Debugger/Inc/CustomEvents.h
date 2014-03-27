
#ifndef _CUSTOMEVENTS_H
#define _CUSTOMEVENTS_H

#include <Windows.h>

// Naming convention:
// First two letters indicate source and destination. GD means Gui to Debug thread.
// Second word indicates what the message is about(tab, menu items, ...)
//
#define GD_TAB_INFOCUS      WM_USER
#define GD_TAB_OUTFOCUS     (WM_USER + 1)
#define GD_MENU_CONTINUE    (WM_USER + 2)
#define GD_MENU_STEPINTO    (WM_USER + 3)
#define GD_MENU_STEPOVER    (WM_USER + 4)
#define GD_MENU_STEPOUT     (WM_USER + 5)
#define GD_MENU_BREAKALL    (WM_USER + 6)
#define GD_MENU_SUSPALL     (WM_USER + 7)
#define GD_MENU_RESALL      (WM_USER + 8)
#define GD_MENU_SUSPRES     (WM_USER + 9)

// Start and end values for use in GetMessage()
#define CUSTOM_EVENT_START  GD_TAB_INFOCUS
#define CUSTOM_EVENT_END    GD_MENU_SUSPRES

#endif // _CUSTOMEVENTS_H
