#include "settings.h"
#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct config_format_entry *e,
		struct config_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);
static void prop_change(struct widget *w, XPropertyEvent *e);
static void client_msg(struct widget *w, XClientMessageEvent *e);

static void dnd_drop(struct widget *w, struct drag_info *di);

static void configure(struct widget *w, XConfigureEvent *e);
static void mouse_motion(struct widget *w, XMotionEvent *e);
static void mouse_leave(struct widget *w);
static void reconfigure(struct widget *w);

struct widget_interface pager_interface = {
	.theme_name		= "pager",
	.size_type		= WIDGET_SIZE_CONSTANT,
	.create_widget_private	= create_widget_private,
	.destroy_widget_private = destroy_widget_private,
	.draw			= draw,
	.button_click		= button_click,
	.prop_change		= prop_change,
	.dnd_drop		= dnd_drop,
	.client_msg		= client_msg,
	.configure		= configure,
	.mouse_motion		= mouse_motion,
	.mouse_leave		= mouse_leave,
	.reconfigure		= reconfigure
};

/**************************************************************************
  Pager theme
**************************************************************************/

#define COLOR_WHITE (unsigned char[]){255,255,255}
#define COLOR_BLACK (unsigned char[]){0,0,0}

static int parse_pager_state(struct pager_state *ps, const char *name,
			     struct config_format_entry *e,
			     struct config_format_tree *tree,
			     int required)
{
	struct config_format_entry *ee = find_config_format_entry(e, name);
	if (!ee) {
		if (required)
			required_entry_not_found(e, name);
		ps->exists = 0;
		return -1;
	}

	parse_color(ps->border, "border", ee, COLOR_WHITE);
	parse_color(ps->fill, "fill", ee, COLOR_BLACK);
	parse_color(ps->inactive_window_border, "inactive_window_border", ee, COLOR_WHITE);
	parse_color(ps->inactive_window_fill, "inactive_window_fill", ee, COLOR_BLACK);
	parse_color(ps->active_window_border, "active_window_border", ee, COLOR_WHITE);
	parse_color(ps->active_window_fill, "active_window_fill", ee, COLOR_BLACK);

	parse_text_info_named(&ps->font, "font", ee, 0);

	ps->exists = 1;
	return 0;
}

static void free_pager_state(struct pager_state *ps)
{
	if (ps->exists)
		free_text_info(&ps->font);
}

static int parse_pager_theme(struct pager_theme *pt,
			     struct config_format_entry *e,
			     struct config_format_tree *tree)
{
	if (parse_pager_state(&pt->states[BUTTON_STATE_IDLE], "idle", e, tree, 1))
		goto parse_pager_state_error_idle;

	if (parse_pager_state(&pt->states[BUTTON_STATE_PRESSED], "pressed", e, tree, 1))
		goto parse_pager_state_error_pressed;

	pt->height = parse_int("height", e, 999);
	pt->desktop_spacing = parse_int("desktop_spacing", e, 1);
	parse_pager_state(&pt->states[BUTTON_STATE_IDLE_HIGHLIGHT],
			  "idle_highlight", e, tree, 0);
	parse_pager_state(&pt->states[BUTTON_STATE_PRESSED_HIGHLIGHT],
			  "pressed_highlight", e, tree, 0);

	return 0;

parse_pager_state_error_pressed:
	free_pager_state(&pt->states[BUTTON_STATE_IDLE]);
parse_pager_state_error_idle:
	return -1;
}

static void free_pager_theme(struct pager_theme *pt)
{
	unsigned int i;
	for (i = 0; i < 4; ++i)
		free_pager_state(&pt->states[i]);
}

/**************************************************************************
  Tasks management
**************************************************************************/

static gboolean task_remove_dead(Window *win, struct pager_task *t, void *notused)
{
	if (t->alive) {
		t->alive = 0;
		return 0;
	}
	xfree(t);
	return 1;
}

static gboolean task_remove_all(Window *win, struct pager_task *t, void *notused)
{
	xfree(t);
	return 1;
}

static void get_window_position(struct x_connection *c, struct pager_task *t, Window win)
{
	XWindowAttributes winattrs;
	XGetWindowAttributes(c->dpy, win, &winattrs);
	t->w = winattrs.width;
	t->h = winattrs.height;
	x_translate_coordinates(c, winattrs.x, winattrs.y, &t->x, &t->y, win);

	long *extents = x_get_prop_data(c, win, c->atoms[XATOM_NET_FRAME_EXTENTS],
					XA_CARDINAL, 0);
	if (extents) {
		t->x -= extents[0]; t->w += extents[0] + extents[1];
		t->y -= extents[2]; t->h += extents[2] + extents[3];
		XFree(extents);
	}
}

static void select_window_input(struct x_connection *c, Window win)
{
	XWindowAttributes winattrs;
	XGetWindowAttributes(c->dpy, win, &winattrs);
	long mask = winattrs.your_event_mask | PropertyChangeMask | StructureNotifyMask;
	XSelectInput(c->dpy, win, mask);
}

static int update_tasks(struct widget *w)
{
	struct x_connection *c = &w->panel->connection;
	struct pager_widget *pw = (struct pager_widget*)w->private;
	if (pw->windows)
		XFree(pw->windows);

	pw->windows = x_get_prop_data(c, c->root, c->atoms[XATOM_NET_CLIENT_LIST_STACKING],
				      XA_WINDOW, &pw->windows_n);
	if (!pw->windows_n)
		return 0;

	int needs_expose = 0;
	size_t i;
	struct pager_task *t;
	for (i = 0; i < pw->windows_n; ++i) {
		Window win = pw->windows[i];
		t = g_hash_table_lookup(pw->tasks, &win);
		if (t) {
			t->alive = 1;
			if (t->stackpos != i) {
				t->stackpos = i;
				needs_expose = 1;
			}
		} else {
			t = xmallocz(sizeof(struct pager_task));
			select_window_input(c, win);
			get_window_position(c, t, win);
			t->win = win;
			t->alive = 1;
			t->desktop = x_get_window_desktop(c, win);
			t->visible = x_is_window_visible_on_screen(c, win);
			t->visible_on_panel = x_is_window_visible_on_panel(c, win);
			t->stackpos = i;

			g_hash_table_insert(pw->tasks, &t->win, t);
			needs_expose = 1;
		}
	}

	g_hash_table_foreach_remove(pw->tasks, (GHRFunc)task_remove_dead, 0);
	return needs_expose;
}

static void clear_tasks(struct pager_widget *pw)
{
	g_hash_table_foreach_remove(pw->tasks, (GHRFunc)task_remove_all, 0);
	g_hash_table_destroy(pw->tasks);
	if (pw->windows)
		XFree(pw->windows);
}

/**************************************************************************
  Desktops management
**************************************************************************/

static void free_desktops(struct pager_widget *pw)
{
	CLEAR_ARRAY(pw->desktops);
}

static void update_active(struct pager_widget *pw, struct x_connection *c)
{
	pw->active_win = x_get_prop_window(c, c->root, c->atoms[XATOM_NET_ACTIVE_WINDOW]);
}

static void update_active_desktop(struct pager_widget *pw, struct x_connection *c)
{
	pw->active = x_get_prop_int(c, c->root,
				    c->atoms[XATOM_NET_CURRENT_DESKTOP]);
}

static void switch_desktop(int desktop, struct x_connection *c)
{
	int desktops = x_get_prop_int(c, c->root,
				      c->atoms[XATOM_NET_NUMBER_OF_DESKTOPS]);
	if (desktop >= desktops)
		return;

	x_send_netwm_message(c, c->root, c->atoms[XATOM_NET_CURRENT_DESKTOP],
			     desktop, 0, 0, 0, 0);
}

static void update_desktops(struct pager_widget *pw, struct x_connection *c)
{
	free_desktops(pw);
	update_active_desktop(pw, c);
	int desktops_n = x_get_prop_int(c, c->root,
					c->atoms[XATOM_NET_NUMBER_OF_DESKTOPS]);
	size_t i;
	for (i = 0; i < desktops_n; ++i) {
		struct pager_desktop d = {0, 0, 0, 0};
		ARRAY_APPEND(pw->desktops, d);
	}
}

static void resize_desktops(struct widget *w)
{
	struct x_connection *c = &w->panel->connection;
	struct pager_widget *pw = (struct pager_widget*)w->private;
	if (pw->theme.height > w->panel->height)
		pw->theme.height = w->panel->height - 2;

	const struct x_monitor *xmon = &c->monitors[w->panel->monitor];
	struct rect mon = {xmon->x, xmon->y, xmon->width, xmon->height};
	if (!pw->current_monitor_only) {
		mon.x = mon.y = 0;
		mon.w = c->screen_width;
		mon.h = c->screen_height;
	}

	int workareas_n = 0;
	long *workareas = x_get_prop_data(c, c->root, c->atoms[XATOM_NET_WORKAREA],
					  XA_CARDINAL, &workareas_n);

	int width = 0;
	size_t i;
	for (i = 0; i < pw->desktops_n; ++i) {
		struct pager_desktop *pd = &pw->desktops[i];
		struct rect workarea = {
			workareas[4*i+0], workareas[4*i+1],
			workareas[4*i+2], workareas[4*i+3]
		};
		rect_intersection(&pd->workarea, &workarea, &mon);
		pd->div = pd->workarea.h / (pw->theme.height - 2);
		pd->w = (mon.w / pd->div) + 2;
		width += pd->w;
	}
	XFree(workareas);

	w->width = width + (pw->desktops_n - 1) * pw->theme.desktop_spacing;
}

static int get_desktop_at(struct widget *w, int x)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;

	size_t i;
	for (i = 0; i < pw->desktops_n; ++i) {
		struct pager_desktop *d = &pw->desktops[i];
		if (x < (d->x + d->w) && x > d->x)
			return (int)i;
	}
	return -1;
}

/**************************************************************************
  Pager interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct config_format_entry *e,
				 struct config_format_tree *tree)
{
	struct pager_widget *pw = xmallocz(sizeof(struct pager_widget));
	if (parse_pager_theme(&pw->theme, e, tree)) {
		xfree(pw);
		XWARNING("Failed to parse pager theme");
		return -1;
	}

	INIT_ARRAY(pw->desktops, 16);
	w->private = pw;

	pw->current_monitor_only = parse_bool("pager_current_monitor_only", &g_settings.root);

	struct x_connection *c = &w->panel->connection;
	update_desktops(pw, c);
	update_active(pw, c);
	resize_desktops(w);
	pw->highlighted = -1;
	pw->tasks = g_hash_table_new(g_int_hash, g_int_equal);
	update_tasks(w);

	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;
	free_pager_theme(&pw->theme);
	free_desktops(pw);
	FREE_ARRAY(pw->desktops);
	clear_tasks(pw);
	xfree(pw);
}

static void draw(struct widget *w)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;
	cairo_t *cr = w->panel->cr;
	PangoLayout *layout = w->panel->layout;
	size_t i;
	struct rect r;
	r.x = w->x;
	r.y = (w->panel->height - pw->theme.height) / 2;
	r.h = pw->theme.height;

	struct rect activerect;
	struct pager_state *activeps = &pw->theme.states[0];

	for (i = 0; i < pw->desktops_n; ++i) {
		struct pager_desktop *pd = &pw->desktops[i];
		int state = (i == pw->active) << 1;
		int state_hl = ((i == pw->active) << 1) | (i == pw->highlighted);
		struct pager_state *ps;

		if (pw->theme.states[state_hl].exists)
			ps = &pw->theme.states[state_hl];
		else
			ps = &pw->theme.states[state];

		pd->x = r.x;
		r.w = pd->w;
		fill_rectangle(cr, ps->fill, &r);

		if (pw->active == i) {
			activerect = r;
			activeps = ps;
		}

		r.x++; r.y++; r.w -= 2; r.h -= 2;

		size_t visible_tasks_count = 0;
		size_t j;
		for (j = 0; j < pw->windows_n; ++j) {
			Window win = pw->windows[j];
			struct pager_task *t = g_hash_table_lookup(pw->tasks, &win);
			if (t && t->visible_on_panel && (t->desktop == i || t->desktop == -1))
				visible_tasks_count++;
			if (t && t->visible && (t->desktop == i || t->desktop == -1)) {
				unsigned char *window_fill;
				unsigned char *window_border;
				struct rect intersection;
				struct rect winr;
				winr.x = r.x + (t->x - pd->workarea.x) / pd->div;
				winr.y = r.y + (t->y - pd->workarea.y) / pd->div;
				winr.w = t->w / pd->div;
				winr.h = t->h / pd->div;
				if (!rect_intersection(&intersection, &winr, &r))
					continue;

				if (win == pw->active_win) {
					window_fill = ps->active_window_fill;
					window_border = ps->active_window_border;
				} else {
					window_fill = ps->inactive_window_fill;
					window_border = ps->inactive_window_border;
				}
				fill_rectangle(cr, window_fill, &intersection);
				draw_rectangle_outline(cr, window_border, &intersection);
			}
		}

		r.x--; r.y--; r.w += 2; r.h += 2;

		draw_rectangle_outline(cr, ps->border, &r);
		if (ps->font.pfd && visible_tasks_count) {
			/* draw number */
			char buf[10];
			snprintf(buf, sizeof(buf), "%u", visible_tasks_count);
			draw_text(cr, layout, &ps->font, buf, r.x, r.y, r.w, r.h, 0);
		}
		r.x += r.w + pw->theme.desktop_spacing;
	}
	draw_rectangle_outline(cr, activeps->border, &activerect);
}

static void button_click(struct widget *w, XButtonEvent *e)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;
	int di = get_desktop_at(w, e->x);
	if (di == -1)
		return;

	struct x_connection *c = &w->panel->connection;

	int mbutton_use = check_mbutton_condition(w->panel, e->button, MBUTTON_USE);

	if (mbutton_use && e->type == ButtonRelease && pw->active != di)
		switch_desktop(di, c);
}

static void prop_change(struct widget *w, XPropertyEvent *e)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	if (e->window == c->root) {
		if (e->atom == c->atoms[XATOM_NET_NUMBER_OF_DESKTOPS]) {
			update_desktops(pw, c);
			resize_desktops(w);
			recalculate_widgets_sizes(w->panel);
			return;
		}

		if (e->atom == c->atoms[XATOM_NET_WORKAREA]) {
			resize_desktops(w);
			recalculate_widgets_sizes(w->panel);
			return;
		}

		if (e->atom == c->atoms[XATOM_NET_ACTIVE_WINDOW]) {
			update_active(pw, c);
			w->needs_expose = 1;
			return;
		}

		if (e->atom == c->atoms[XATOM_NET_CURRENT_DESKTOP]) {
			update_active_desktop(pw, c);
			w->needs_expose = 1;
			return;
		}

		if (e->atom == c->atoms[XATOM_NET_CLIENT_LIST_STACKING]) {
			int needs = update_tasks(w);
			if (!w->needs_expose)
				w->needs_expose = needs;
			return;
		}
	}

	struct pager_task *t = g_hash_table_lookup(pw->tasks, &e->window);
	if (!t)
		return;

	if (e->atom == c->atoms[XATOM_NET_WM_DESKTOP]) {
		t->desktop = x_get_window_desktop(c, t->win);
		w->needs_expose = 1;
		return;
	}

	if (e->atom == c->atoms[XATOM_NET_WM_STATE]) {
		t->visible = x_is_window_visible_on_screen(c, t->win);
		t->visible_on_panel = x_is_window_visible_on_panel(c, t->win);
		w->needs_expose = 1;
		return;
	}

	if (e->atom == c->atoms[XATOM_NET_FRAME_EXTENTS]) {
		get_window_position(c, t, e->window);
		w->needs_expose = 1;
		return;
	}
}

static void client_msg(struct widget *w, XClientMessageEvent *e)
{
	struct panel *p = w->panel;
	struct x_connection *c = &p->connection;
	struct pager_widget *pw = (struct pager_widget*)w->private;

	if (e->message_type == c->atoms[XATOM_XDND_POSITION]) {
		int x = (e->data.l[2] >> 16) & 0xFFFF;

		/* if it's not ours, skip.. */
		if ((x < (p->x + w->x)) || (x > (p->x + w->x + w->width)))
			return;

		int di = get_desktop_at(w, x - p->x);
		if (di != -1 && di != pw->active)
				switch_desktop(di, c);

		x_send_dnd_message(c, e->data.l[0],
				   c->atoms[XATOM_XDND_STATUS],
				   p->win,
				   2, /* bits: 0 1 */
				   0, 0,
				   None);
	}
}

static void dnd_drop(struct widget *w, struct drag_info *di)
{
	if (di->taken_on->interface != &taskbar_interface)
		return;

	struct taskbar_widget *tw = di->taken_on->private;
	if (tw->taken == None)
		return;

	int desktop = get_desktop_at(w, di->dropped_x);
	if (desktop == -1)
		return;

	struct x_connection *c = &w->panel->connection;
	x_send_netwm_message(c, tw->taken,
			     c->atoms[XATOM_NET_WM_DESKTOP],
			     (long)desktop, 2, 0, 0, 0);
}

static void configure(struct widget *w, XConfigureEvent *e)
{
	struct x_connection *c = &w->panel->connection;
	struct pager_widget *pw = (struct pager_widget*)w->private;
	struct pager_task *t = g_hash_table_lookup(pw->tasks, &e->window);
	if (!t)
		return;

	get_window_position(c, t, e->window);
	w->needs_expose = 1;
}

static void mouse_motion(struct widget *w, XMotionEvent *e)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;
	int i = get_desktop_at(w, e->x);
	if (i != pw->highlighted) {
		pw->highlighted = i;
		w->needs_expose = 1;
	}
}

static void mouse_leave(struct widget *w)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;
	if (pw->highlighted != -1) {
		pw->highlighted = -1;
		w->needs_expose = 1;
	}
}

static void reconfigure(struct widget *w)
{
	struct pager_widget *pw = (struct pager_widget*)w->private;
	int current_monitor_only = parse_bool("pager_current_monitor_only", &g_settings.root);
	if (current_monitor_only != pw->current_monitor_only) {
		pw->current_monitor_only = current_monitor_only;
		resize_desktops(w);
		recalculate_widgets_sizes(w->panel);
	}
}
