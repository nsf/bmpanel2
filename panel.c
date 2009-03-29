#include <string.h>
#include <stdio.h>
#include "gui.h"

/**************************************************************************
  Panel theme
**************************************************************************/

static int parse_position(const char *pos)
{
	if (strcmp("top", pos) == 0)
		return PANEL_POSITION_TOP;
	else if (strcmp("bottom", pos) == 0)
		return PANEL_POSITION_BOTTOM;
	xwarning("Unknown position type: %s, back to default 'top'", pos);
	return PANEL_POSITION_TOP;
}

static int load_panel_theme(struct panel_theme *theme, struct theme_format_tree *tree)
{
	memset(theme, 0, sizeof(struct panel_theme));
	struct theme_format_entry *e = theme_format_find_entry(&tree->root, "panel");
	if (!e)
		return xerror("Failed to find 'panel' section in theme format file");

	const char *v;
	struct theme_format_entry *ee;

	theme->position = PANEL_POSITION_TOP; /* default */
	v = theme_format_find_entry_value(e, "position");
	if (v)
		theme->position = parse_position(v);
	
	theme->separator.img = 0; /* default */
	ee = theme_format_find_entry(e, "separator");
	if (ee) 
		parse_image_part(&theme->separator, ee, tree);

	/* background is necessary */
	ee = theme_format_find_entry(e, "background");
	if (!ee || parse_image_part(&theme->background, ee, tree) != 0) {
		if (theme->separator.img)
			release_image(theme->separator.img);
		return xerror("Missing 'background' image in panel section");
	}

	theme->height = cairo_image_surface_get_height(theme->background.img->surface);

	return 0;
}

static void free_panel_theme(struct panel_theme *theme)
{
	release_image(theme->background.img);
	if (theme->separator.img)
		release_image(theme->separator.img);
}

/**************************************************************************
  Panel
**************************************************************************/

static void get_position_and_strut(const struct x_connection *c, 
		const struct panel_theme *t, int *ox, int *oy, 
		int *ow, int *oh, long *strut)
{
	int x,y,w,h;
	x = c->workarea_x;
	y = c->workarea_y;
	h = t->height;
	w = c->workarea_width;

	strut[0] = strut[1] = strut[3] = 0;
	strut[2] = h + c->workarea_y;
	if (t->position == PANEL_POSITION_BOTTOM) {
		y = (c->workarea_height + c->workarea_y) - h;
		strut[2] = 0;
		strut[3] = h + c->screen_height - 
			(c->workarea_height + c->workarea_y);
	}

	*ox = x; *oy = y; *oh = h; *ow = w;
}

static Window create_window(struct x_connection *c, struct panel_theme *t)
{
	int x,y,w,h;
	long strut[4];
	long tmp;
	Window win;
	XSetWindowAttributes attrs;
	attrs.background_pixel = 0xFF0000;

	get_position_and_strut(c, t, &x, &y, &w, &h, strut);
	win = XCreateWindow(c->dpy, c->root, x, y, w, h, 0, 
			c->default_depth, InputOutput, 
			c->default_visual, CWBackPixel, &attrs);
	if (win == None)
		return None;

	/* NETWM struts */
	XChangeProperty(c->dpy, win, c->atoms[XATOM_NET_WM_STRUT], XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char*)strut, 4);

	static const struct {
		int s, e;
	} where[] = {
		[PANEL_POSITION_TOP] = {8, 9},
		[PANEL_POSITION_BOTTOM] = {10, 11}
	};

	long strutp[12] = {strut[0], strut[1], strut[2], strut[3],};

	strutp[where[t->position].s] = x;
	strutp[where[t->position].e] = x+w;
	XChangeProperty(c->dpy, win, c->atoms[XATOM_NET_WM_STRUT_PARTIAL], 
			XA_CARDINAL, 32, PropModeReplace, (unsigned char*)strutp, 12);

	/* desktops */
	tmp = -1;
	XChangeProperty(c->dpy, win, c->atoms[XATOM_NET_WM_DESKTOP], XA_CARDINAL, 
			32, PropModeReplace, (unsigned char*)&tmp, 1);

	/* window type */
	tmp = c->atoms[XATOM_NET_WM_WINDOW_TYPE_DOCK];
	XChangeProperty(c->dpy, win, c->atoms[XATOM_NET_WM_WINDOW_TYPE], XA_ATOM, 
			32, PropModeReplace, (unsigned char*)&tmp, 1);

	/* place window on it's position */
	XSizeHints size_hints;

	size_hints.x = x;
	size_hints.y = y;
	size_hints.width = w;
	size_hints.height = h;

	size_hints.flags = PPosition | PMaxSize | PMinSize;
	size_hints.min_width = size_hints.max_width = w;
	size_hints.min_height = size_hints.max_height = h;
	XSetWMNormalHints(c->dpy, win, &size_hints);

	/* motif hints */
	#define MWM_HINTS_DECORATIONS (1L << 1)
	struct mwmhints {
		uint32_t flags;
		uint32_t functions;
		uint32_t decorations;
		int32_t input_mode;
		uint32_t status;
	} mwm = {MWM_HINTS_DECORATIONS,0,0,0,0};
	XChangeProperty(c->dpy, win, c->atoms[XATOM_MOTIF_WM_HINTS], 
			c->atoms[XATOM_MOTIF_WM_HINTS], 32, PropModeReplace, 
			(unsigned char*)&mwm, sizeof(struct mwmhints) / 4);
	#undef MWM_HINTS_DECORATIONS

	/* set classhint */
	XClassHint *ch;
	if ((ch = XAllocClassHint())) {
		ch->res_name = "panel";
		ch->res_class = "bmpanel";
		XSetClassHint(c->dpy, win, ch);
		XFree(ch);
	}

	return win;
}

static int parse_panel_widgets(struct panel *panel, struct theme_format_tree *tree)
{
	size_t i;
	for (i = 0; i < tree->root.children_n; ++i) {
		struct theme_format_entry *e = &tree->root.children[i];
		struct widget_interface *we = lookup_widget_interface(e->name);
		if (we) {
			if (panel->widgets_n == PANEL_MAX_WIDGETS)
				return xerror("error: Widgets limit reached");
			struct widget *w = (*we->create_widget)(e, tree);
			if (w) 
				panel->widgets[panel->widgets_n++] = w;
			else
				xwarning("Failed to create widget: %s", e->name);
		}
	}
	return 0;
}

int panel_create(struct panel *panel, struct theme_format_tree *tree)
{
	memset(panel, 0, sizeof(struct panel));

	/* connect to X server */
	if (x_connect(&panel->connection, 0)) {
		xwarning("Failed to connect to X server");
		goto panel_create_error_x;
	}

	/* parse panel theme */
	if (load_panel_theme(&panel->theme, tree)) {
		xwarning("Failed to load theme format file");
		goto panel_create_error_theme;
	}

	struct x_connection *c = &panel->connection;
	struct panel_theme *t = &panel->theme;

	/* create window */
	panel->win = create_window(c, t);
	if (panel->win == None) {
		xwarning("Can't create panel window");
		goto panel_create_error_win;
	}

	/* parse panel widgets */
	if (parse_panel_widgets(panel, tree)) {
		xwarning("Failed to load one of panel's widgets");
		goto panel_create_error_widgets;
	}

	/* all ok, map window */
	XMapWindow(c->dpy, panel->win);
	XSync(c->dpy, 0);

	return 0;

panel_create_error_widgets:
	XDestroyWindow(c->dpy, panel->win);
panel_create_error_win:
	free_panel_theme(&panel->theme);
panel_create_error_theme:
	x_disconnect(&panel->connection);
panel_create_error_x:
	return -1;
}

void panel_destroy(struct panel *panel)
{
	size_t i;
	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *w = panel->widgets[i];
		(*w->interface->destroy_widget)(w);
	}

	XDestroyWindow(panel->connection.dpy, panel->win);
	free_panel_theme(&panel->theme);
	x_disconnect(&panel->connection);
}
