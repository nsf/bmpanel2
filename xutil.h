#ifndef BMPANEL2_XUTIL_H 
#define BMPANEL2_XUTIL_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "util.h"

enum {
	XATOM_WM_STATE,
	XATOM_NET_DESKTOP_NAMES,
	XATOM_NET_WM_STATE,
	XATOM_NET_ACTIVE_WINDOW,
	XATOM_NET_WM_NAME,
	XATOM_NET_WM_ICON_NAME,
	XATOM_NET_WM_VISIBLE_ICON_NAME,
	XATOM_NET_WORKAREA,
	XATOM_NET_WM_ICON,
	XATOM_NET_WM_VISIBLE_NAME,
	XATOM_NET_WM_STATE_SKIP_TASKBAR,
	XATOM_NET_WM_STATE_SHADED,
	XATOM_NET_WM_STATE_HIDDEN,
	XATOM_NET_WM_DESKTOP,
	XATOM_NET_MOVERESIZE_WINDOW,
	XATOM_NET_WM_WINDOW_TYPE,
	XATOM_NET_WM_WINDOW_TYPE_DOCK,
	XATOM_NET_WM_WINDOW_TYPE_DESKTOP,
	XATOM_NET_WM_STRUT,
	XATOM_NET_WM_STRUT_PARTIAL,
	XATOM_NET_CLIENT_LIST,
	XATOM_NET_NUMBER_OF_DESKTOPS,
	XATOM_NET_CURRENT_DESKTOP,
	XATOM_NET_SYSTEM_TRAY_OPCODE,
	XATOM_UTF8_STRING,
	XATOM_MOTIF_WM_HINTS,
	XATOM_XROOTPMAP_ID,
	XATOM_COUNT
};

struct x_connection {
	Display *dpy;

	int screen;
	int screen_width;
	int screen_height;

	int workarea_x;
	int workarea_y;
	int workarea_width;
	int workarea_height;

	Visual *default_visual;
	Colormap default_colormap;
	int default_depth;

	Window root;
	Pixmap root_pixmap;
	
	Atom atoms[XATOM_COUNT];
};

int x_connect(struct x_connection *c, const char *display);
void x_disconnect(struct x_connection *c);

int x_get_prop_int(struct x_connection *c, Window win, Atom at);
Window x_get_prop_window(struct x_connection *c, Window win, Atom at);
Pixmap x_get_prop_pixmap(struct x_connection *c, Window win, Atom at);
int x_get_window_desktop(struct x_connection *c, Window win);

#endif /* BMPANEL2_XUTIL_H */
