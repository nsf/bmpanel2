#include <stdio.h>
#include "gui.h"
#include "settings.h"
#include "widget-utils.h"

/**************************************************************************
  Panel theme
**************************************************************************/

static int parse_position(const char *pos)
{
	if (strcmp("top", pos) == 0)
		return PANEL_POSITION_TOP;
	else if (strcmp("bottom", pos) == 0)
		return PANEL_POSITION_BOTTOM;
	else if (strcmp("left", pos) == 0)
		return PANEL_POSITION_LEFT;
	else if (strcmp("right", pos) == 0)
		return PANEL_POSITION_RIGHT;
	XWARNING("Unknown position type: %s, back to default 'top'", pos);
	return PANEL_POSITION_BOTTOM;
}

static int load_panel_theme(struct panel_theme *theme, struct config_format_tree *tree)
{
	CLEAR_STRUCT(theme);
	struct config_format_entry *e = find_config_format_entry(&tree->root, "panel");
	if (!e)
		return XERROR("Failed to find 'panel' section in theme format file");

	theme->position = PANEL_POSITION_BOTTOM; /* default */
	const char *v = find_config_format_entry_value(e, "position");
	if (v)
		theme->position = parse_position(v);
	
	if ( theme->position == PANEL_POSITION_LEFT ||
	     theme->position == PANEL_POSITION_RIGHT )
		theme->vertical = 1;
	
	theme->background = parse_image_part_named("background", e, tree, 1);
	if (!theme->background)
		return -1;

	theme->separator = parse_image_part_named("separator", e, tree, 0);
	theme->transparent = parse_bool("transparent", e);
	theme->align = parse_align("align", e);
	theme->width = parse_int_or_percents("width", e, -1, 
					     &theme->width_in_percents);
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

static void select_render_interface(struct panel *p)
{
	/* TODO: composite manager detection and composite render */
	if (p->theme.transparent)
		p->render = &render_pseudo;
	else
		p->render = &render_normal;
}

static void get_position_and_strut(
		const struct x_connection *c, 
		const struct panel_theme *t,
		int *ox, int *oy, int *ow, int *oh,
		long *strut)
{
	int x,y,w,h;
	
	/* strut:
	 *   left, right, top, bottom,
	 *   left_start_y, left_end_y,
	 *   right_start_y, right_end_y,
	 *   top_start_x, top_end_x,
	 *   bottom_start_x, bottom_end_x
	 */
	memset(strut, 0, 12 * sizeof(strut[0])); /* set zeroes */
	
	if ( !t->vertical ) { /* horizontal */
		h = image_height(t->background);
		w = c->workarea_width;
		if (t->width > 0) {
			if (t->width_in_percents)
				w = ((float)c->workarea_width / 100.0f) * t->width;
			else
				w = t->width;
			/* limit */
			if (w > c->workarea_width)
				w = c->workarea_width;
		}
		
		/* alignment */
		switch (t->align) {
		case ALIGN_CENTER:
			x = c->workarea_x + (c->workarea_width - w) / 2;
			break;
		case ALIGN_RIGHT:
			x = c->workarea_x + c->workarea_width - w;
			break;
		default:
			x = c->workarea_x;
			break;
		}
	}
	else { /* vertical */
		h = c->workarea_height;
		if (t->width > 0) {
			if (t->width_in_percents)
				h = ((float)c->workarea_height / 100.0f) * t->width;
			else
				w = t->width;
			/* limit */
			if (w > c->workarea_height)
				w = c->workarea_height;
		}
		w = image_width(t->background);
		
		/* alignment */
		switch (t->align) {
		case ALIGN_CENTER:
			y = c->workarea_y + (c->workarea_height - h) / 2;
			break;
		case ALIGN_RIGHT:
			y = c->workarea_y + c->workarea_height - h;
			break;
		default:
			y = c->workarea_y;
			break;
		}
	}
	
	if ( t->position == PANEL_POSITION_TOP ) {
		y = c->workarea_y;
		
		strut[2] = c->workarea_y + h; // top
		
		strut[8] = x;     // top_start_x
		strut[9] = x + w; // top_end_x
	}
	else if (t->position == PANEL_POSITION_BOTTOM) {
		y = (c->workarea_height + c->workarea_y) - h;
		
		strut[3] = h + c->screen_height - 
			(c->workarea_height + c->workarea_y); // bottom
		
		strut[10] = x;     // bottom_start_x
		strut[11] = x + w; // bottom_end_x
	}
	else if ( t->position == PANEL_POSITION_LEFT ) {
		x = c->workarea_x;
		
		strut[0] = image_width(t->background); // left
		
		strut[4] = y;     // left_start_x
		strut[5] = y + h; // left_end_x
	}
	else if (t->position == PANEL_POSITION_RIGHT) {
		x = (c->workarea_width + c->workarea_x) - w;
		
		strut[1] = w + c->screen_width - 
			(c->workarea_width + c->workarea_x); // right
		
		strut[6] = y;     // right_start_x
		strut[7] = y + h; // right_end_x
	}

	*ox = x; *oy = y; *oh = h; *ow = w;
}

static void create_window(struct panel *panel)
{
	struct x_connection *c = &panel->connection;
	struct panel_theme *t = &panel->theme;

	int x,y,w,h;
	long strut[12] = {0};
	
	get_position_and_strut(c, t, &x, &y, &w, &h, strut);

	(*panel->render->create_win)(panel, x, y, w, h, 
			ExposureMask | StructureNotifyMask | ButtonPressMask |
			ButtonReleaseMask | PointerMotionMask | EnterWindowMask |
			LeaveWindowMask);

	panel->x = x;
	panel->y = y;
	panel->width = w;
	panel->height = h;

	/* Xdnd awareness */
	x_set_prop_atom(c, panel->win, c->atoms[XATOM_XDND_AWARE], 5);

	/* XWMHints */
	XWMHints wmhints;
	wmhints.flags = InputHint;
	wmhints.input = 0;
	XSetWMHints(c->dpy, panel->win, &wmhints);

	/* NETWM struts */
	x_set_prop_array(c, panel->win, c->atoms[XATOM_NET_WM_STRUT], strut, 4);
	x_set_prop_array(c, panel->win, c->atoms[XATOM_NET_WM_STRUT_PARTIAL], 
			strut, 12);

	/* desktops and window type */
	x_set_prop_int(c, panel->win, c->atoms[XATOM_NET_WM_DESKTOP], -1);
	x_set_prop_atom(c, panel->win, c->atoms[XATOM_NET_WM_WINDOW_TYPE],
			c->atoms[XATOM_NET_WM_WINDOW_TYPE_DOCK]);
	
	Atom state[2] = {c->atoms[XATOM_NET_WM_STATE_STICKY], c->atoms[XATOM_NET_WM_STATE_BELOW]};
	x_set_prop_atom_array(c, panel->win, c->atoms[XATOM_NET_WM_STATE], state, 2);

	/* also send desktop message to wm (on all desktops) */
	x_send_netwm_message(c, panel->win, c->atoms[XATOM_NET_WM_DESKTOP], 
			     0xFFFFFFFF, 0, 0, 0, 0);

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

	/* classhint */
	XClassHint ch;
	ch.res_name = "panel";
	ch.res_class = "bmpanel";
	XSetClassHint(c->dpy, panel->win, &ch);
}

static void parse_panel_widgets(struct panel *panel, struct config_format_tree *tree)
{
	size_t i;
	for (i = 0; i < tree->root.children_n; ++i) {
		struct config_format_entry *e = &tree->root.children[i];
		struct widget_interface *we = lookup_widget_interface(e->name);
		if (we) {
			if (panel->widgets_n == PANEL_MAX_WIDGETS)
				XDIE("error: Widgets limit reached");
			
			struct widget *w = &panel->widgets[panel->widgets_n];

			w->interface = we;
			w->panel = panel;
			w->needs_expose = 0;

			if ((*we->create_widget_private)(w, e, tree) == 0) {
				panel->widgets_n++;
				w->no_separator = parse_bool("no_separator", e);
			} else {
				XWARNING("Failed to create widget: \"%s\"", e->name);
			}
		}
	}
}

void recalculate_widgets_sizes(struct panel *panel)
{
	const int min_fill_size = 200;
	short vertical = panel->theme.vertical;
	int num_constant = 0;
	int num_fill = 0;
	int total_constants_size = 0;
	int pos = 0;
	int pos2 = vertical ? panel->height : panel->width;
	int separators = 0;
	int separator_size = vertical ? image_height(panel->theme.separator) : image_width(panel->theme.separator);
	int total_separators_size = 0;
	size_t i;

	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *w = &panel->widgets[i];
		if (w->interface->size_type == WIDGET_SIZE_CONSTANT) {
			int w_size = vertical ? w->height : w->width;
			num_constant++;
			total_constants_size += w_size;
			if (w_size && !w->no_separator)
				separators++;
		} else
			num_fill++;
	}

	total_separators_size = separators * separator_size;

	if (num_fill != 1)
		XDIE("There always should be exactly one widget with a "
		     "SIZE_FILL size type (taskbar)");

	if (total_constants_size + total_separators_size > 
	    (vertical ? panel->height : panel->width) - min_fill_size)
	{
		XDIE("Too many widgets here, try to remove one or more");
	}

	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *w = &panel->widgets[i];
		int w_size = vertical ? w->height : w->width;
		if (w->interface->size_type == WIDGET_SIZE_FILL)
			break;
		
		if ( vertical ) {
			w->x = 0;
			w->y = pos;
		} else {
			w->x = pos;
			w->y = 0;
		}
		pos += w_size;
		if (w_size && !w->no_separator)
			pos += separator_size;
	}

	for (i = panel->widgets_n - 1;; --i) {
		struct widget *w = &panel->widgets[i];
		int w_size = vertical ? w->height : w->width;
		if (w->interface->size_type == WIDGET_SIZE_FILL)
			break;

		pos2 -= w_size;
		if ( vertical ) {
			w->x = 0;
			w->y = pos2;
		} else {
			w->x = pos2;
			w->y = 0;
		}
		if (w_size && !w->no_separator)
			pos2 -= separator_size;
	}
	
	/* Set SIZE_FILL widget size */
	struct widget *w = &panel->widgets[i];
	if ( vertical ) {
		w->x = 0;
		w->y = pos;
		w->height = pos2 - pos;
	}
	else {
		w->x = pos;
		w->y = 0;
		w->width = pos2 - pos;
	}
	
	/* request redraw */
	panel->needs_expose = 1;
}

static void expose_whole_panel(struct panel *panel)
{
	Display *dpy = panel->connection.dpy;
	short vertical = panel->theme.vertical;
	
	int sepw = 0;
	sepw += vertical ? image_height(panel->theme.separator) : image_width(panel->theme.separator);

	size_t i;
	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *wi = &panel->widgets[i];
		int x = wi->x;
		int y = wi->y;
		int w = wi->width;
		int h = wi->height;
		if (!w || !h) /* skip empty */
			continue;

		/* background */
		pattern_image(panel->theme.background, panel->cr, x, y, w, h);
		
		/* widget contents */
		if (wi->interface->draw)
			(*wi->interface->draw)(wi);

		/* separator */
		if ( vertical )
			y += h;
		else
			x += w;
		if (panel->theme.separator && panel->widgets_n - 1 != i) { /* separator & not last */
			blit_image(panel->theme.separator, panel->cr, x, y);
		}

		/* widget was drawn, clear "needs_expose" flag */
		wi->needs_expose = 0;
	}

	(*panel->render->blit)(panel, 0, 0, panel->width, panel->height);
	XFlush(dpy);
	panel->needs_expose = 0;

	/* after exposing panel actions, for those who need panel background
	 * (e.g. systray icons)
	 */
	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *wi = &panel->widgets[i];
		if (wi->interface->panel_exposed)
			(*wi->interface->panel_exposed)(wi);
	}
	XFlush(dpy);
}

static void expose_panel(struct panel *panel)
{
	if (panel->needs_expose) {
		expose_whole_panel(panel);
		return;
	}

	size_t i;
	Display *dpy = panel->connection.dpy;
	for (i = 0; i < panel->widgets_n; ++i) {
		struct widget *w = &panel->widgets[i];
		if (w->needs_expose) {
			pattern_image(panel->theme.background, panel->cr, w->x, w->y, w->width, w->height);
			if (w->interface->draw)
				(*w->interface->draw)(w);
			(*panel->render->blit)(panel, w->x, w->y,
					w->width, w->height);
			w->needs_expose = 0;
		}
	}
	XFlush(dpy);
}

void init_panel(struct panel *panel, struct config_format_tree *tree)
{
	CLEAR_STRUCT(panel);

	/* connect to X server */
	x_connect(&panel->connection, 0);

	/* parse panel theme */
	if (load_panel_theme(&panel->theme, tree))
		XDIE("Failed to load theme format file");
	
	panel->drag_threshold = parse_int("drag_threshold",
					  &g_settings.root, 30);

	select_render_interface(panel);
	struct x_connection *c = &panel->connection;

	/* create window */
	create_window(panel);
	
	/* render private */
	if (panel->render->create_private)
		(*panel->render->create_private)(panel);

	/* rendering context */
	(*panel->render->create_dc)(panel);

	/* create text layout */
	panel->layout = pango_cairo_create_layout(panel->cr);

	/* parse panel widgets */
	parse_panel_widgets(panel, tree);
	recalculate_widgets_sizes(panel);

	/* all ok, map window */
	expose_panel(panel);
	XMapWindow(c->dpy, panel->win);
	XFlush(c->dpy);
	
	/* send desktop property again after mapping (fluxbox bug?) */
	x_send_netwm_message(c, panel->win, c->atoms[XATOM_NET_WM_DESKTOP], 
			0xFFFFFFFF, 0, 0, 0, 0);
}

void free_panel(struct panel *panel)
{
	size_t i;

	if (panel->render->free_private)
		(*panel->render->free_private)(panel);

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

static void panel_property_notify(struct panel *p, XPropertyEvent *e)
{
	if (e->atom == p->connection.atoms[XATOM_XROOTPMAP_ID] &&
	    p->render->update_bg) 
	{
		(*p->render->update_bg)(p);
	}
}

static void panel_expose(struct panel *p, XExposeEvent *e)
{
	if (e->window == p->win && p->render->expose)
		(*p->render->expose)(p);
}

static int process_events(struct panel *p)
{
	Display *dpy = p->connection.dpy;
	int events_processed = 0;
	
	while (XPending(dpy)) {
		XEvent e;

		events_processed++;
		XNextEvent(dpy, &e);

		switch (e.type) {
		
		case NoExpose:
		case MapNotify:
		case UnmapNotify:
		case VisibilityNotify:
		case ReparentNotify:
			/* skip? */
			break;

		case Expose:
			panel_expose(p, &e.xexpose);
			break;
		
		case ButtonRelease:
		case ButtonPress:
			disp_button_press_release(p, &e.xbutton);
			break;

		case MotionNotify:
			disp_motion_notify(p, &e.xmotion);
			break;

		case EnterNotify:
		case LeaveNotify:
			disp_enter_leave_notify(p, &e.xcrossing);
			break;
		
		case PropertyNotify:
			panel_property_notify(p, &e.xproperty);
			disp_property_notify(p, &e.xproperty);
			break;

		case ClientMessage:
			disp_client_msg(p, &e.xclient);
			break;
		
		case ConfigureNotify:
			disp_configure(p, &e.xconfigure);
			break;

		case DestroyNotify:
			disp_win_destroy(p, &e.xdestroywindow);
			break;
		
		default:
			XWARNING("Unknown XEvent (type: %d, win: %d)", 
				 e.type, e.xany.window);
			break;
		}
	}
	if (events_processed)
		expose_panel(p);
	return events_processed;
}

static gboolean panel_second_timeout(gpointer data)
{
	struct panel *p = data;
	size_t i;
	struct widget *w;
	for (i = 0; i < p->widgets_n; ++i) {
		w = &p->widgets[i];
		if (w->interface->clock_tick)
			(*w->interface->clock_tick)(w);
	}
	expose_panel(p);
	/* just in case, actually it helps a lot */
	process_events(p);
	return TRUE;
}

static gboolean panel_x_in(GIOChannel *gio, GIOCondition condition, gpointer data)
{
	/* TODO: be aware of connection drop */
	/* ENSURE(condition == G_IO_IN, "Input condition failed"); */
	struct panel *p = data;

	/* we do here more greedy processing */
	while (process_events(p))
		;

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

