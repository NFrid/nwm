#ifndef VARIABLES_H_
#define VARIABLES_H_

#include "drw.h"
#include "nwm.h"

extern Systray*   systray;     // systray
extern const char broken[];    // mark for broken clients
extern char       stext[1024]; // text in a statusbar
extern int        screen;      // screen
extern int        sw, sh;      // X display screen geometry
extern int        bh, blw;     // bar geometry
extern int        th;          // tab bar geometry
extern int        lrpad;       // sum of left and right padding for text
extern int (*xerrorxlib)(Display*, XErrorEvent*);

extern Atom     wmatom[WMLast];   // wm-specific atoms
extern Atom     netatom[NetLast]; // _NET atoms
extern Atom     xatom[XLast];     // x-specific atoms
extern int      running;          // 0 means nwm stops
extern Cur*     cursor[CurLast];  // used cursors
extern Clr**    scheme;           // colorscheme
extern Display* dpy;              // display of X11 session
extern Drw*     drw;              // TODO: wtf is drawer (I guess it draws 🗿)
extern Monitor* mons;             // all the monitors
extern Monitor* selmon;           // selected monitor
extern Window   root;             // root window
extern Window   wmcheckwin;       // window for NetWMCheck

#endif // VARIABLES_H_