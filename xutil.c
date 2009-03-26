#include <string.h>
#include "xutil.h"

static char *atom_names[] = {
	"WM_STATE",
	"_NET_DESKTOP_NAMES",
	"_NET_WM_STATE",
	"_NET_ACTIVE_WINDOW",
	"_NET_WM_NAME",
	"_NET_WM_ICON_NAME",
	"_NET_WM_VISIBLE_ICON_NAME",
	"_NET_WORKAREA",
	"_NET_WM_ICON",
	"_NET_WM_VISIBLE_NAME",
	"_NET_WM_STATE_SKIP_TASKBAR",
	"_NET_WM_STATE_SHADED",
	"_NET_WM_STATE_HIDDEN",
	"_NET_WM_DESKTOP",
	"_NET_MOVERESIZE_WINDOW",
	"_NET_WM_WINDOW_TYPE",
	"_NET_WM_WINDOW_TYPE_DOCK",
	"_NET_WM_WINDOW_TYPE_DESKTOP",
	"_NET_WM_STRUT",
	"_NET_WM_STRUT_PARTIAL",
	"_NET_CLIENT_LIST",
	"_NET_NUMBER_OF_DESKTOPS",
	"_NET_CURRENT_DESKTOP",
	"_NET_SYSTEM_TRAY_OPCODE",
	"UTF8_STRING",
	"_MOTIF_WM_HINTS",
	"_XROOTPMAP_ID"
};

static void *get_prop_data(Display *dpy, Window win, Atom prop, Atom type, int *items)
{
	Atom type_ret;
	int format_ret;
	unsigned long items_ret;
	unsigned long after_ret;
	unsigned char *prop_data;

	prop_data = 0;

	XGetWindowProperty(dpy, win, prop, 0, 0x7fffffff, False,
			type, &type_ret, &format_ret, &items_ret,
			&after_ret, &prop_data);
	if (items)
		*items = items_ret;

	return prop_data;
}

int x_get_prop_int(struct x_connection *c, Window win, Atom at)
{
	int num = 0;
	long *data;

	data = get_prop_data(c->dpy, win, at, XA_CARDINAL, 0);
	if (data) {
		num = *data;
		XFree(data);
	}
	return num;
}

Window x_get_prop_window(struct x_connection *c, Window win, Atom at)
{
	Window num = 0;
	Window *data;

	data = get_prop_data(c->dpy, win, at, XA_WINDOW, 0);
	if (data) {
		num = *data;
		XFree(data);
	}
	return num;
}

Pixmap x_get_prop_pixmap(struct x_connection *c, Window win, Atom at)
{
	Pixmap num = 0;
	Pixmap *data;

	data = get_prop_data(c->dpy, win, at, XA_PIXMAP, 0);
	if (data) {
		num = *data;
		XFree(data);
	}
	return num;
}

int x_get_window_desktop(struct x_connection *c, Window win)
{
	return x_get_prop_int(c, win, c->atoms[XATOM_NET_WM_DESKTOP]);
}

int x_connect(struct x_connection *c, const char *display)
{
	memset(c, 0, sizeof(struct x_connection));
	c->dpy = XOpenDisplay(display);
	if (!c->dpy)
		return 1;
	
	/* get internal atoms */
	XInternAtoms(c->dpy, atom_names, XATOM_COUNT, False, c->atoms);

	c->screen 		= DefaultScreen(c->dpy);
	c->screen_width 	= DisplayWidth(c->dpy, c->screen);
	c->screen_height 	= DisplayHeight(c->dpy, c->screen);
	c->workarea_x 		= 0;
	c->workarea_y 		= 0;
	c->workarea_width 	= c->screen_width;
	c->workarea_height 	= c->screen_height;
	c->default_visual 	= DefaultVisual(c->dpy, c->screen);
	c->default_colormap 	= DefaultColormap(c->dpy, c->screen);
	c->default_depth 	= DefaultDepth(c->dpy, c->screen);
	c->root 		= RootWindow(c->dpy, c->screen);
	c->root_pixmap 		= x_get_prop_pixmap(c, c->root, c->atoms[XATOM_XROOTPMAP_ID]);

	/* get workarea */
	long *workarea = get_prop_data(c->dpy, c->root, c->atoms[XATOM_NET_WORKAREA], XA_CARDINAL, 0);
	if (workarea) {
		c->workarea_x = workarea[0];
		c->workarea_y = workarea[1];
		c->workarea_width = workarea[2];
		c->workarea_height = workarea[3];
		XFree(workarea);	
	}

	return 0;
}

void x_disconnect(struct x_connection *c)
{
	XCloseDisplay(c->dpy);
}
