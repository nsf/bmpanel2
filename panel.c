#include <string.h>
#include <stdio.h>
#include "gui.h"
#include "parsing-utils.h"

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
	struct theme_format_entry *e = find_theme_format_entry(&tree->root, "panel");
	if (!e)
		return xerror("Failed to find 'panel' section in theme format file");

	const char *v;
	struct theme_format_entry *ee;

	theme->position = PANEL_POSITION_TOP; /* default */
	v = find_theme_format_entry_value(e, "position");
	if (v)
		theme->position = parse_position(v);
	
	theme->background = parse_image_part_named("background", e, tree);
	if (!theme->background)
		return xerror("Missing 'background' image in panel section");

	theme->separator = parse_image_part_named("separator", e, tree);

	return 0;
}

static void free_panel_theme(struct panel_theme *theme)
{
	cairo_surface_destroy(theme->background);
	if (theme->separator)
		cairo_surface_destroy(theme->separator);
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
	h = cairo_image_surface_get_height(t->background);
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

static int create_window(struct panel *panel)
{
	struct x_connection *c = &panel->connection;
	struct panel_theme *t = &panel->theme;

	int x,y,w,h;
	long strut[4];
	long tmp;
	Window win;
	XSetWindowAttributes attrs;
	
	get_position_and_strut(c, t, &x, &y, &w, &h, strut);

	panel->bg = XCreatePixmap(c->dpy, c->root, w, h, c->default_depth);
	if (panel->bg == None)
		return xerror("Failed to create background pixmap");
	
	attrs.background_pixmap = None;

	panel->win = XCreateWindow(c->dpy, c->root, x, y, w, h, 0, 
			c->default_depth, InputOutput, 
			c->default_visual, CWBackPixmap, &attrs);
	if (panel->win == None) {
		XFreePixmap(c->dpy, panel->bg);
		return xerror("Failed to create window");
	}

	XSelectInput(c->dpy, panel->win, 
			ButtonPressMask | ButtonReleaseMask | ExposureMask | 
			StructureNotifyMask);

	panel->x = x;
	panel->y = y;
	panel->width = w;
	panel->height = h;

	/* NETWM struts */
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_STRUT], XA_CARDINAL, 32, 
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
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_STRUT_PARTIAL], 
			XA_CARDINAL, 32, PropModeReplace, (unsigned char*)strutp, 12);

	/* desktops */
	tmp = -1;
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_DESKTOP], XA_CARDINAL, 
			32, PropModeReplace, (unsigned char*)&tmp, 1);

	/* window type */
	tmp = c->atoms[XATOM_NET_WM_WINDOW_TYPE_DOCK];
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_WINDOW_TYPE], XA_ATOM, 
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
	XSetWMNormalHints(c->dpy, panel->win, &size_hints);

	/* motif hints */
	#define MWM_HINTS_DECORATIONS (1L << 1)
	struct mwmhints {
		uint32_t flags;
		uint32_t functions;
		uint32_t decorations;
		int32_t input_mode;
		uint32_t status;
	} mwm = {MWM_HINTS_DECORATIONS,0,0,0,0};
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_MOTIF_WM_HINTS], 
			c->atoms[XATOM_MOTIF_WM_HINTS], 32, PropModeReplace, 
			(unsigned char*)&mwm, sizeof(struct mwmhints) / 4);
	#undef MWM_HINTS_DECORATIONS

	/* set classhint */
	XClassHint *ch;
	if ((ch = XAllocClassHint())) {
		ch->res_name = "panel";
		ch->res_class = "bmpanel";
		XSetClassHint(c->dpy, panel->win, ch);
		XFree(ch);
	}

	return 0;
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
			
			struct widget *w = &panel->widgets[panel->widgets_n];
			w->interface = we;
			w->panel = panel;
			if ((*we->create_widget_private)(w, e, tree) == 0)
				panel->widgets_n++;
			else
				xwarning("Failed to create widget: %s", e->name);
		}
	}
	return 0;
}

static int calculate_widget_sizes(struct panel *panel)
{
	const int min_fill_size = 200;
	int num_constant = 0;
	int num_fill = 0;
	int total_constants_width = 0;
	int x = 0;
	int x2 = panel->width;
	size_t i;

	for (i = 0; i < panel->widgets_n; ++i) {
		if (panel->widgets[i].interface->size_type == WIDGET_SIZE_CONSTANT) {
			num_constant++;
			total_constants_width += panel->widgets[i].width;
		} else
			num_fill++;

		panel->widgets[i].y = 0;
		panel->widgets[i].height = panel->height;
	}

	if (num_fill != 1)
		return xerror("There always should be one widget with SIZE_FILL");

	if (total_constants_width > panel->width - min_fill_size)
		return xerror("Too many widgets here, try to remove one or more");

	for (i = 0; i < panel->widgets_n; ++i) {
		if (panel->widgets[i].interface->size_type == WIDGET_SIZE_FILL)
			break;

		panel->widgets[i].x = x;
		x += panel->widgets[i].width;
	}

	for (i = panel->widgets_n - 1; i >= 0; --i) {
		if (panel->widgets[i].interface->size_type == WIDGET_SIZE_FILL)
			break;

		x2 -= panel->widgets[i].width;
		panel->widgets[i].x = x2;
	}

	panel->widgets[i].x = x;
	panel->widgets[i].width = x2 - x;

	return 0;
}

static int create_drawing_context(struct panel *panel)
{
	struct x_connection *c = &panel->connection;

	cairo_surface_t *bgs = cairo_xlib_surface_create(c->dpy, 
			panel->bg, c->default_visual, 
			panel->width, panel->height);

	if (cairo_surface_status(bgs) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(bgs);
		return xerror("Failed to create cairo surface");
	}

	panel->cr = cairo_create(bgs);
	cairo_surface_destroy(bgs);

	if (cairo_status(panel->cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(panel->cr);
		return xerror("Failed to create cairo drawing context");
	}

	pattern_image(panel->theme.background, panel->cr, 0, 0, panel->width);

	return 0;
}

int create_panel(struct panel *panel, struct theme_format_tree *tree)
{
	size_t i;
	memset(panel, 0, sizeof(struct panel));

	/* connect to X server */
	if (x_connect(&panel->connection, 0)) {
		xwarning("Failed to connect to X server");
		goto create_panel_error_x;
	}

	/* parse panel theme */
	if (load_panel_theme(&panel->theme, tree)) {
		xwarning("Failed to load theme format file");
		goto create_panel_error_theme;
	}

	struct x_connection *c = &panel->connection;
	struct panel_theme *t = &panel->theme;

	/* create window */
	if (create_window(panel)) {
		xwarning("Can't create panel window");
		goto create_panel_error_win;
	}

	/* parse panel widgets */
	if (parse_panel_widgets(panel, tree)) {
		xwarning("Failed to load one of panel's widgets");
		goto create_panel_error_widgets;
	}

	if (calculate_widget_sizes(panel)) {
		xwarning("Failed to calculate widgets sizes");
		goto create_panel_error_widget_sizes;
	}

	if (create_drawing_context(panel)) {
		xwarning("Failed to create drawing context");
		goto create_panel_error_drawing_context;
	}

	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *w = &panel->widgets[i];
		(*w->interface->draw)(w);
	}

	/* doesn't fail? */
	panel->layout = pango_cairo_create_layout(panel->cr);

	/* all ok, map window */
	XMapWindow(c->dpy, panel->win);
	XSync(c->dpy, 0);

	expose_panel(panel, 0, 0, panel->width, panel->height);

	return 0;

create_panel_error_drawing_context:
create_panel_error_widget_sizes:
	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *w = &panel->widgets[i];
		(*w->interface->destroy_widget_private)(w);
	}
create_panel_error_widgets:
	XDestroyWindow(c->dpy, panel->win);
	XFreePixmap(c->dpy, panel->bg);
create_panel_error_win:
	free_panel_theme(&panel->theme);
create_panel_error_theme:
	x_disconnect(&panel->connection);
create_panel_error_x:
	return -1;
}

void destroy_panel(struct panel *panel)
{
	size_t i;
	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *w = &panel->widgets[i];
		(*w->interface->destroy_widget_private)(w);
	}

	g_object_unref(panel->layout);
	cairo_destroy(panel->cr);
	XDestroyWindow(panel->connection.dpy, panel->win);
	XFreePixmap(panel->connection.dpy, panel->bg);
	free_panel_theme(&panel->theme);
	x_disconnect(&panel->connection);
}

int point_in_rect(int px, int py, int x, int y, int w, int h)
{
	return (px > x &&
		px < x + w &&
		py > y &&
		py < y + h);
}

gboolean panel_second_timeout(gpointer data)
{
}

static gboolean panel_x_in(GIOChannel *gio, GIOCondition condition, gpointer data)
{
	struct panel *p = data;
	Display *dpy = p->connection.dpy;
	size_t i;
	struct widget *w;

	while (XPending(dpy)) {
		XEvent e;
		XNextEvent(dpy, &e);

		switch (e.type) {
		case Expose:
			expose_panel(p, 0, 0, p->width, p->height);
			break;
		case ButtonRelease:
		case ButtonPress:
			for (i = 0; i < p->widgets_n; ++i) {
				w = &p->widgets[i];
				if (w->interface->button_click &&
				    point_in_rect(e.xbutton.x, e.xbutton.y,
						w->x, w->y, w->width, w->height))
				{
					(*w->interface->button_click)(w, &e.xbutton);
				}
			}
			break;
		case PropertyNotify:
			for (i = 0; i < p->widgets_n; ++i) {
				w = &p->widgets[i];
				if (w->interface->prop_change)
					(*w->interface->prop_change)(w, &e.xproperty);
			}
			break;
		default:
			xwarning("Unknown XEvent (type: %d)", e.type);
			break;
		}
	}
	return 1;
}

void panel_main_loop(struct panel *panel)
{
	int fd = ConnectionNumber(panel->connection.dpy);
	panel->loop = g_main_loop_new(0, 0);
	
	GIOChannel *x = g_io_channel_unix_new(fd);
	g_io_add_watch(x, G_IO_IN | G_IO_HUP, panel_x_in, panel);
	g_timeout_add(1000, panel_second_timeout, panel);

	g_main_loop_run(panel->loop);
	g_main_destroy(panel->loop);
}

void expose_panel(struct panel *panel, int x, int y, int w, int h)
{
	Display *dpy = panel->connection.dpy;
	GC dgc = panel->connection.default_gc;
	XCopyArea(dpy, panel->bg, panel->win, dgc, x, y, w, h, x, y);
	XSync(dpy, 0);
}
