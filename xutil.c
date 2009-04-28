#include "xutil.h"

static char *atom_names[] = {
	"WM_STATE",
	"_NET_DESKTOP_NAMES",
	"_NET_WM_STATE",
	"_NET_ACTIVE_WINDOW",
	"_NET_CLOSE_WINDOW",
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

void *x_get_prop_data(struct x_connection *c, Window win, Atom prop, Atom type, int *items)
{
	Atom type_ret;
	int format_ret;
	unsigned long items_ret;
	unsigned long after_ret;
	unsigned char *prop_data;

	prop_data = 0;

	XGetWindowProperty(c->dpy, win, prop, 0, 0x7fffffff, False,
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

	data = x_get_prop_data(c, win, at, XA_CARDINAL, 0);
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

	data = x_get_prop_data(c, win, at, XA_WINDOW, 0);
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

	data = x_get_prop_data(c, win, at, XA_PIXMAP, 0);
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
	CLEAR_STRUCT(c);
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

	XSelectInput(c->dpy, c->root, PropertyChangeMask);

	/* get workarea */
	long *workarea = x_get_prop_data(c, c->root, c->atoms[XATOM_NET_WORKAREA], XA_CARDINAL, 0);
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

Window x_create_default_window(struct x_connection *c, int x, int y, 
		unsigned int w, unsigned int h, unsigned long valuemask,
		XSetWindowAttributes *attrs)
{
	return XCreateWindow(c->dpy, c->root, x, y, w, h, 0, c->default_depth,
			InputOutput, c->default_visual, valuemask, attrs);
}

Pixmap x_create_default_pixmap(struct x_connection *c, unsigned int w,
		unsigned int h)
{
	return XCreatePixmap(c->dpy, c->root, w, h, c->default_depth);
}

void x_set_prop_int(struct x_connection *c, Window win, Atom type, int value)
{
	XChangeProperty(c->dpy, win, type, XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char*)&value, 1);
}

void x_set_prop_atom(struct x_connection *c, Window win, Atom type, Atom at)
{
	XChangeProperty(c->dpy, win, type, XA_ATOM, 32, 
			PropModeReplace, (unsigned char*)&at, 1);
}

void x_set_prop_array(struct x_connection *c, Window win, Atom type, 
		const long *values, size_t len)
{
	XChangeProperty(c->dpy, win, type, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char*)values, len);
}

int x_is_window_hidden(struct x_connection *c, Window win)
{
	Atom *data;
	int ret = 0;
	int num;

	data = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_WINDOW_TYPE], 
			XA_ATOM, &num);
	if (data) {
		while (num) {
			num--;
			if (data[num] == c->atoms[XATOM_NET_WM_WINDOW_TYPE_DOCK] ||
			    data[num] == c->atoms[XATOM_NET_WM_WINDOW_TYPE_DESKTOP]) 
			{
				XFree(data);
				return 1;
			}

		}
		XFree(data);
	}

	data = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_STATE], XA_ATOM, &num);
	if (!data)
		return 0;

	while (num) {
		num--;
		if (data[num] == c->atoms[XATOM_NET_WM_STATE_SKIP_TASKBAR])
			ret = 1;
	}
	XFree(data);

	return ret;
}

int x_is_window_iconified(struct x_connection *c, Window win)
{
	unsigned long *data;
	int ret = 0;

	data = x_get_prop_data(c, win, c->atoms[XATOM_WM_STATE], 
	     		c->atoms[XATOM_WM_STATE], 0);
	if (data) {
		if (data[0] == IconicState) {
			ret = 1;
		}
		XFree(data);
	}

	int num;
	data = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_STATE], XA_ATOM, &num);
	if (data) {
		while (num) {
			num--;
			if (data[num] == c->atoms[XATOM_NET_WM_STATE_HIDDEN])
				ret = 1;
		}
		XFree(data);
	}

	return ret;
}

char *x_alloc_window_name(struct x_connection *c, Window win)
{
	char *ret, *name = 0;
	name = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_VISIBLE_ICON_NAME], 
			c->atoms[XATOM_UTF8_STRING], 0);
	if (name) 
		goto name_here;
	name = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_ICON_NAME], 
			c->atoms[XATOM_UTF8_STRING], 0);
	if (name) 
		goto name_here;
	name = x_get_prop_data(c, win, XA_WM_ICON_NAME, XA_STRING, 0);
	if (name) 
		goto name_here;
	name = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_VISIBLE_NAME], 
			c->atoms[XATOM_UTF8_STRING], 0);
	if (name) 
		goto name_here;
	name = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_NAME],
			c->atoms[XATOM_UTF8_STRING], 0);
	if (name) 
		goto name_here;
	name = x_get_prop_data(c, win, XA_WM_NAME, XA_STRING, 0);
	if (name) 
		goto name_here;

	return xstrdup("<unknown>");
name_here:
	ret = xstrdup(name);
	XFree(name);
	return ret;
}

void x_send_netwm_message(struct x_connection *c, Window win,
		Atom a, long l0, long l1, long l2, long l3, long l4)
{
	XClientMessageEvent e;

	e.type = ClientMessage;
	e.window = win;
	e.message_type = a;
	e.format = 32;
	e.data.l[0] = l0;
	e.data.l[1] = l1;
	e.data.l[2] = l2;
	e.data.l[3] = l3;
	e.data.l[4] = l4;

	XSendEvent(c->dpy, c->root, False, SubstructureNotifyMask |
			SubstructureRedirectMask, (XEvent*)&e);
}
