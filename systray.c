#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
		struct config_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void client_msg(struct widget *w, XClientMessageEvent *e);
static void win_destroy(struct widget *w, XDestroyWindowEvent *e);
static void configure(struct widget *w, XConfigureEvent *e);
static void panel_exposed(struct widget *w);

struct widget_interface systray_interface = {
	.theme_name 		= "systray",
	.size_type 		= WIDGET_SIZE_CONSTANT,
	.create_widget_private 	= create_widget_private,
	.destroy_widget_private = destroy_widget_private,
	.client_msg 		= client_msg,
	.win_destroy 		= win_destroy,
	.configure		= configure,
	.panel_exposed		= panel_exposed
};

/**************************************************************************
  Tray management
**************************************************************************/

static Atom acquire_tray_selection_atom(struct x_connection *c)
{
	char systray_atom[32];
	snprintf(systray_atom, sizeof(systray_atom), "_NET_SYSTEM_TRAY_S%u", c->screen);

	return XInternAtom(c->dpy, systray_atom, False);
}

static int tray_selection_owner_exists(struct x_connection *c, Atom traysel)
{
	Window old_owner = XGetSelectionOwner(c->dpy, traysel);
	if (old_owner != 0) {
		return 1;
	}
	return 0;
}

static void add_tray_icon(struct widget *w, Window win)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	struct systray_icon icon;
	icon.mapped = 0;
	icon.icon = win;

	/* create embedder window */
	XSetWindowAttributes attrs;
	attrs.background_pixmap = ParentRelative;
	icon.embedder = XCreateWindow(c->dpy, w->panel->win, 0, 0, 
				      sw->icon_size[0], sw->icon_size[1], 
				      0, c->default_depth, InputOutput, 
				      c->default_visual, 
				      CWBackPixmap, &attrs);

	/* Select structure notifications. Some tray icons require double 
	 * size sets (I don't know why, but it works).
	 */
	XSelectInput(c->dpy, icon.icon, StructureNotifyMask);

	XResizeWindow(c->dpy, icon.icon, sw->icon_size[0], sw->icon_size[1]);
	XReparentWindow(c->dpy, icon.icon, icon.embedder, 0, 0);
	XMapRaised(c->dpy, icon.icon);

	ARRAY_APPEND(sw->icons, icon);
}

static void update_systray_width(struct widget *w)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	w->width = sw->icons_n * sw->icon_size[0];
}

static int find_tray_icon(struct systray_widget *sw, Window win)
{
	size_t i;
	for (i = 0; i < sw->icons_n; ++i) {
		if (sw->icons[i].icon == win)
			return (int)i;
	}
	return -1;
}

static void free_tray_icon(struct widget *w, Window win)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	int i = find_tray_icon(sw, win);
	if (i != -1) {
		XDestroyWindow(c->dpy, sw->icons[i].embedder);
		ARRAY_REMOVE(sw->icons, i);
	}
}

static void free_tray_icons(struct widget *w)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	size_t i;
	for (i = 0; i < sw->icons_n; ++i) {
		struct systray_icon *ic = &sw->icons[i];
		XReparentWindow(c->dpy, ic->icon, c->root, 0, 0);
		XDestroyWindow(c->dpy, ic->embedder);
	}
	FREE_ARRAY(sw->icons);
}

/**************************************************************************
  Systray interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
		struct config_format_tree *tree)
{
	struct systray_widget *sw = xmallocz(sizeof(struct systray_widget));
	if (parse_2ints(sw->icon_size, "icon_size", e) != 0) {
		xfree(sw);
		/* TODO: print this error more nicely */
		required_entry_not_found(e, "icon_size");
		return -1;
	}

	struct x_connection *c = &w->panel->connection;

	sw->tray_selection_atom = acquire_tray_selection_atom(c);
	if (tray_selection_owner_exists(c, sw->tray_selection_atom)) {
		xfree(sw);
		XWARNING("There is another tray selection owner, disabling tray");
		return -1;
	}

	INIT_ARRAY(sw->icons, 20);
	sw->selection_owner = x_create_default_window(c, 0, 0, 1, 1, 0, 0);
	XSetSelectionOwner(c->dpy, sw->tray_selection_atom, sw->selection_owner,
			   CurrentTime);
	
	/* inform other clients that we're here */
	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.message_type = XInternAtom(c->dpy, "MANAGER", False);
	ev.xclient.display = c->dpy;
	ev.xclient.window = c->root;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = CurrentTime; /* timestamp */
	ev.xclient.data.l[1] = sw->tray_selection_atom; /* manager selection atom */
	ev.xclient.data.l[2] = sw->selection_owner; /* the window owning the selection */
	ev.xclient.data.l[3] = 0l; /* selection specific data */
	ev.xclient.data.l[4] = 0l; /* selection specific data */

	XSendEvent(c->dpy, c->root, False, StructureNotifyMask, &ev);

	w->width = 0;
	w->private = sw;

	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	struct x_connection *c = &w->panel->connection;
	free_tray_icons(w);
	XSetSelectionOwner(c->dpy, sw->tray_selection_atom, None, CurrentTime);
	XDestroyWindow(c->dpy, sw->selection_owner);
	xfree(sw);
}

#define TRAY_REQUEST_DOCK 0

static void client_msg(struct widget *w, XClientMessageEvent *e)
{
	struct x_connection *c = &w->panel->connection;

	if (e->message_type == c->atoms[XATOM_NET_SYSTEM_TRAY_OPCODE] &&
	    e->data.l[1] == TRAY_REQUEST_DOCK) 
	{
		add_tray_icon(w, e->data.l[2]);
		update_systray_width(w);
		recalculate_widgets_sizes(w->panel);
	}
}

static void panel_exposed(struct widget *w)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	size_t i;
	int x = w->x;
	int y = (w->panel->height - sw->icon_size[1]) / 2;
	for (i = 0; i < sw->icons_n; ++i) {
		XMoveResizeWindow(c->dpy, sw->icons[i].embedder, x, y, 
				  sw->icon_size[0], sw->icon_size[1]);
		if (!sw->icons[i].mapped) {
			XMapRaised(c->dpy, sw->icons[i].embedder);
			sw->icons[i].mapped = 1;
		}
		XClearArea(c->dpy, sw->icons[i].icon, 0,0,0,0, True);

		x += sw->icon_size[0];
	}
}

static void win_destroy(struct widget *w, XDestroyWindowEvent *e)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	size_t icons_n = sw->icons_n;

	free_tray_icon(w, e->window);
	if (icons_n != sw->icons_n) {
		update_systray_width(w);
		recalculate_widgets_sizes(w->panel);
	}
}

static void configure(struct widget *w, XConfigureEvent *e)
{
	struct systray_widget *sw = (struct systray_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	int i = find_tray_icon(sw, e->window);
	if (i != -1) {
		XWindowChanges wc;
		wc.width = sw->icon_size[0];
		wc.height = sw->icon_size[1];
		XConfigureWindow(c->dpy, e->window, CWWidth | CWHeight, &wc);
	}
}
