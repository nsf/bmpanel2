#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);
static void prop_change(struct widget *w, XPropertyEvent *e);

static void mouse_enter(struct widget *w);
static void mouse_leave(struct widget *w);

static void dnd_start(struct widget *w, struct drag_info *di);
static void dnd_drag(struct widget *w, struct drag_info *di);
static void dnd_drop(struct widget *w, struct drag_info *di);

static struct widget_interface taskbar_interface = {
	"taskbar",
	WIDGET_SIZE_FILL,
	create_widget_private,
	destroy_widget_private,
	draw,
	button_click,
	0, /* clock_tick */
	prop_change,
	0, /* mouse_enter */
	0, /* mouse_leave */
	0, /* mouse_motion */
	dnd_start,
	dnd_drag,
	dnd_drop
};

void register_taskbar()
{
	register_widget_interface(&taskbar_interface);
}

/**************************************************************************
  Taskbar theme
**************************************************************************/

static int parse_taskbar_state(struct taskbar_state *ts, const char *name,
		struct theme_format_entry *e, struct theme_format_tree *tree)
{
	struct theme_format_entry *ee = find_theme_format_entry(e, name);
	if (!ee)
		return -1;

	if (parse_triple_image(&ts->background, ee, tree))
		goto parse_taskbar_state_error_background;

	if (parse_text_info(&ts->font, "font", ee)) {
		XWARNING("Failed to parse \"font\" element in taskbar "
			 "button state theme");
		goto parse_taskbar_state_error_font;
	}

	return 0;

parse_taskbar_state_error_font:
	free_triple_image(&ts->background);
parse_taskbar_state_error_background:
	return -1;
}

static void free_taskbar_state(struct taskbar_state *ts)
{
	free_triple_image(&ts->background);
	free_text_info(&ts->font);
}

static int parse_taskbar_theme(struct taskbar_theme *tt, 
		struct theme_format_entry *e, struct theme_format_tree *tree)
{
	if (parse_taskbar_state(&tt->idle, "idle", e, tree)) {
		XWARNING("Failed to parse \"idle\" taskbar button state theme");
		goto parse_taskbar_button_theme_error_idle;
	}

	if (parse_taskbar_state(&tt->pressed, "pressed", e, tree)) {
		XWARNING("Failed to parse \"pressed\" taskbar button state theme");
		goto parse_taskbar_button_theme_error_pressed;
	}

	struct theme_format_entry *ee = find_theme_format_entry(e, "default_icon");
	if (ee) {
		tt->default_icon = parse_image_part(ee, tree);
		tt->icon_offset[0] = tt->icon_offset[1] = 0;
		parse_2ints(tt->icon_offset, "offset", ee);
	}

	tt->separator = parse_image_part_named("separator", e, tree);

	return 0;

parse_taskbar_button_theme_error_pressed:
	free_taskbar_state(&tt->idle);
parse_taskbar_button_theme_error_idle:
	return -1;
}

static void free_taskbar_theme(struct taskbar_theme *tt)
{
	free_taskbar_state(&tt->idle);
	free_taskbar_state(&tt->pressed);
	if (tt->default_icon)
		cairo_surface_destroy(tt->default_icon);
	if (tt->separator)
		cairo_surface_destroy(tt->separator);
}

/**************************************************************************
  Taskbar task management
**************************************************************************/

static int find_task_by_window(struct taskbar_widget *tw, Window win)
{
	size_t i;
	for (i = 0; i < tw->tasks_n; ++i) {
		if (tw->tasks[i].win == win)
			return (int)i;
	}
	return -1;
}

static int find_last_task_by_desktop(struct taskbar_widget *tw, int desktop)
{
	int t = -1;
	size_t i;
	for (i = 0; i < tw->tasks_n; ++i) {
		if (tw->tasks[i].desktop <= desktop)
			t = (int)i;
	}
	return t;
}

static void add_task(struct taskbar_widget *tw, struct x_connection *c, Window win)
{
	struct taskbar_task t;

	if (x_is_window_hidden(c, win))
		return;

	XSelectInput(c->dpy, win, PropertyChangeMask);

	t.win = win;
	t.name = x_alloc_window_name(c, win); 
	if (tw->theme.default_icon)
		t.icon = get_window_icon(c, win, tw->theme.default_icon);
	else
		t.icon = 0;
	t.desktop = x_get_window_desktop(c, win);



	int i = find_last_task_by_desktop(tw, t.desktop);
	if (i == -1)
		ARRAY_APPEND(tw->tasks, t);
	else
		ARRAY_INSERT_AFTER(tw->tasks, i, t);
}

static void remove_task(struct taskbar_widget *tw, size_t i)
{
	xfree(tw->tasks[i].name);
	if (tw->tasks[i].icon)
		cairo_surface_destroy(tw->tasks[i].icon);
	ARRAY_REMOVE(tw->tasks, i);
}

static void free_tasks(struct taskbar_widget *tw)
{
	size_t i;
	for (i = 0; i < tw->tasks_n; ++i) {
		xfree(tw->tasks[i].name);
		if (tw->tasks[i].icon)
			cairo_surface_destroy(tw->tasks[i].icon);
	}
	FREE_ARRAY(tw->tasks);
}

static int count_tasks_on_desktop(struct taskbar_widget *tw, int desktop)
{
	int count = 0;
	size_t i;
	for (i = 0; i < tw->tasks_n; ++i) {
		if (tw->tasks[i].desktop == desktop ||
		    tw->tasks[i].desktop == -1) /* count "all desktops" too */
		{
			count++;
		}
	}
	return count;
}

static void draw_task(struct taskbar_task *task, struct taskbar_theme *theme,
		cairo_t *cr, PangoLayout *layout, int x, int w, int active)
{
	/* calculations */
	struct triple_image *tbt = (active) ? 
			&theme->pressed.background :
			&theme->idle.background;
	struct text_info *font = (active) ?
			&theme->pressed.font :
			&theme->idle.font;
	int leftw = 0;
	int centerw = 0;
	int rightw = 0;
	int height = cairo_image_surface_get_height(tbt->center);
	if (tbt->left)
		leftw = cairo_image_surface_get_width(tbt->left);
	if (tbt->right)
		rightw = cairo_image_surface_get_width(tbt->right);
	centerw = w - leftw - rightw;
	
	int iconw = 0;
	int iconh = 0;
	if (theme->default_icon) {
		iconw = cairo_image_surface_get_width(theme->default_icon);
		iconh = cairo_image_surface_get_height(theme->default_icon);
	}

	int textw = centerw - (iconw + theme->icon_offset[0]) - rightw;

	/* background */
	int xx = x;
	if (leftw)
		blit_image(tbt->left, cr, xx, 0);
	xx += leftw;
	pattern_image(tbt->center, cr, xx, 0, centerw);
	xx += centerw;
	if (rightw)
		blit_image(tbt->right, cr, xx, 0);
	xx -= centerw;

	/* icon */
	if (iconw && iconh) {
		int yy = (height - iconh) / 2;
		xx += theme->icon_offset[0];
		yy += theme->icon_offset[1];
		blit_image(task->icon, cr, xx, yy);
	}
	xx += iconw; 
	
	/* text */
	draw_text(cr, layout, font, task->name, xx, 0, textw, height);
}

static void send_netwm_message(struct x_connection *c, struct taskbar_task *t,
		Atom a, long l0, long l1, long l2, long l3, long l4)
{
	XClientMessageEvent e;

	e.type = ClientMessage;
	e.window = t->win;
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

static inline void activate_task(struct x_connection *c, struct taskbar_task *t)
{
	send_netwm_message(c, t, c->atoms[XATOM_NET_ACTIVE_WINDOW], 
			2, CurrentTime, 0, 0, 0);
}

static inline void close_task(struct x_connection *c, struct taskbar_task *t)
{
	send_netwm_message(c, t, c->atoms[XATOM_NET_CLOSE_WINDOW],
			CurrentTime, 2, 0, 0, 0);
}

static void move_task(struct taskbar_widget *tw, int what, int where)
{
	struct taskbar_task t = tw->tasks[what];
	if (what == where)
		return;
	ARRAY_REMOVE(tw->tasks, what);
	if (where > what) {
		where -= 1;
		ARRAY_INSERT_AFTER(tw->tasks, where, t);
	} else {
		ARRAY_INSERT_BEFORE(tw->tasks, where, t);
	}
}

/**************************************************************************
  Updates
**************************************************************************/

static void update_active(struct taskbar_widget *tw, struct x_connection *c)
{
	tw->active = x_get_prop_window(c, c->root, 
			c->atoms[XATOM_NET_ACTIVE_WINDOW]);
}

static void update_desktop(struct taskbar_widget *tw, struct x_connection *c)
{
	tw->desktop = x_get_prop_int(c, c->root, 
			c->atoms[XATOM_NET_CURRENT_DESKTOP]);
}

static void update_tasks(struct taskbar_widget *tw, struct x_connection *c)
{
	Window *wins;
	int num;

	wins = x_get_prop_data(c, c->root, c->atoms[XATOM_NET_CLIENT_LIST], 
			XA_WINDOW, &num);

	size_t i, j;
	for (i = 0; i < tw->tasks_n; ++i) {
		struct taskbar_task *t = &tw->tasks[i];
		int delete = 1;
		for (j = 0; j < num; ++j) {
			if (wins[j] == t->win) {
				delete = 0;
				break;
			}
		}
		if (delete)
			remove_task(tw, i--);
	}

	for (i = 0; i < num; ++i) {
		if (find_task_by_window(tw, wins[i]) == -1)
			add_task(tw, c, wins[i]);
	}
	
	XFree(wins);
}

/**************************************************************************
  Taskbar interface
**************************************************************************/

int get_taskbar_task_at(struct widget *w, int x, int y)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;

	size_t i;
	for (i = 0; i < tw->tasks_n; ++i) {
		struct taskbar_task *t = &tw->tasks[i];
		if (t->desktop != tw->desktop &&
		    t->desktop != -1)
		{
			continue;
		}

		if (x < (t->x + t->w) && x > t->x)
			return (int)i;
	}
	return -1;
}

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree)
{
	struct taskbar_widget *tw = xmallocz(sizeof(struct taskbar_widget));
	if (parse_taskbar_theme(&tw->theme, e, tree)) {
		xfree(tw);
		return -1;
	}

	INIT_ARRAY(tw->tasks, 50);
	w->private = tw;

	struct x_connection *c = &w->panel->connection;
	update_desktop(tw, c);
	update_active(tw, c);
	update_tasks(tw, c);
	tw->dnd_win = None;

	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	free_taskbar_theme(&tw->theme);
	free_tasks(tw);
	xfree(tw);
}

static void draw(struct widget *w)
{
	/* I think it's a good idea to calculate all buttons positions here, and
	 * cache these data for later use in other message handlers. User
	 * interacts with what he/she sees, right? 
	 */
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	cairo_t *cr = w->panel->cr;

	int count = count_tasks_on_desktop(tw, tw->desktop);
	if (!count)
		return;

	int taskw = w->width / count;
	int x = w->x;
	size_t i;

	for (i = 0; i < tw->tasks_n; ++i) {
		struct taskbar_task *t = &tw->tasks[i];
		
		if (t->desktop != tw->desktop &&
		    t->desktop != -1)
		{
			continue;
		}

		/* last task width correction */
		if (i == tw->tasks_n - 1)
			taskw = (w->x + w->width) - x;
		
		/* save position for other events */
		t->x = x;
		t->w = taskw;

		draw_task(t, &tw->theme, cr, w->panel->layout,
				x, taskw, t->win == tw->active);
		x += taskw;
	}
}

static void prop_change(struct widget *w, XPropertyEvent *e)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	/* root window props */
	if (e->window == c->root) {
		if (e->atom == c->atoms[XATOM_NET_ACTIVE_WINDOW]) {
			update_active(tw, c);
			w->needs_expose = 1;
			return;
		}
		if (e->atom == c->atoms[XATOM_NET_CURRENT_DESKTOP]) {
			update_desktop(tw, c);
			w->needs_expose = 1;
			return;
		}
		if (e->atom == c->atoms[XATOM_NET_CLIENT_LIST]) {
			update_tasks(tw, c);
			w->needs_expose = 1;
			return;
		}
	}

	/* check if it's our task */
	int ti = find_task_by_window(tw, e->window);
	if (ti == -1)
		return;

	/* desktop changed (task was moved to other desktop) */
	if (e->atom == c->atoms[XATOM_NET_WM_DESKTOP]) {
		struct taskbar_task t = tw->tasks[ti];
		t.desktop = x_get_window_desktop(c, t.win);

		ARRAY_REMOVE(tw->tasks, ti);
		int insert_after = find_last_task_by_desktop(tw, t.desktop);
		if (insert_after == -1)
			ARRAY_APPEND(tw->tasks, t);
		else
			ARRAY_INSERT_AFTER(tw->tasks, insert_after, t);
		w->needs_expose = 1;
		return;
	}

	/* task name was changed */
	if (e->atom == c->atoms[XATOM_NET_WM_VISIBLE_ICON_NAME] ||
	    e->atom == c->atoms[XATOM_NET_WM_ICON_NAME] || 
	    e->atom == XA_WM_ICON_NAME || 
	    e->atom == c->atoms[XATOM_NET_WM_VISIBLE_NAME] ||
	    e->atom == c->atoms[XATOM_NET_WM_NAME] || 
	    e->atom == XA_WM_NAME)
	{
		struct taskbar_task *t = &tw->tasks[ti];
		xfree(t->name);
		t->name = x_alloc_window_name(c, t->win);
		w->needs_expose = 1;
		return;
	}

	/* icon was changed */
	if (tw->theme.default_icon) {
		if (e->atom == c->atoms[XATOM_NET_WM_ICON] ||
		    e->atom == XA_WM_HINTS)
		{
			struct taskbar_task *t = &tw->tasks[ti];
			cairo_surface_destroy(t->icon);
			t->icon = get_window_icon(c, t->win, tw->theme.default_icon);
			w->needs_expose = 1;
			return;
		}
	}
}

static void button_click(struct widget *w, XButtonEvent *e)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	int ti = get_taskbar_task_at(w, e->x, e->y);
	if (ti == -1)
		return;
	struct taskbar_task *t = &tw->tasks[ti];
	struct x_connection *c = &w->panel->connection;

	if (e->button == 1 && e->type == ButtonRelease) {
		if (tw->active == t->win) {
			XIconifyWindow(c->dpy, t->win, c->screen);
		} else {
			activate_task(c, t);
			
			XWindowChanges wc;
			wc.stack_mode = Above;
			XConfigureWindow(c->dpy, t->win, CWStackMode, &wc);
		}
	}
	if (e->button == 2) {
		g_main_loop_quit(w->panel->loop);
	}
}

static void dnd_start(struct widget *w, struct drag_info *di)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	struct x_connection *c = &w->panel->connection;
	struct panel *p = w->panel;

	int ti = get_taskbar_task_at(di->taken_on, di->taken_x, di->taken_y);
	if (ti == -1)
		return;

	struct taskbar_task *t = &tw->tasks[ti];
	
	if (t->icon) {
		int iconw = cairo_image_surface_get_width(t->icon);
		int iconh = cairo_image_surface_get_width(t->icon);
		Pixmap bg = x_create_default_pixmap(c, iconw, iconh);
		cairo_surface_t *surface = cairo_xlib_surface_create(c->dpy,
				bg, c->default_visual, iconw, iconh);
		cairo_t *cr = cairo_create(surface);
		cairo_surface_destroy(surface);

		blit_image(t->icon, cr, 0, 0);
		cairo_destroy(cr);
	
		XSetWindowAttributes attrs;
		attrs.background_pixmap = bg;
		attrs.override_redirect = True;
		
		tw->dnd_win = x_create_default_window(c, di->cur_root_x, 
				di->cur_root_y, iconw, iconh, 
				CWOverrideRedirect | CWBackPixmap, &attrs);
		XFreePixmap(c->dpy, bg);
		XMapWindow(c->dpy, tw->dnd_win);
	}

	Cursor cur = XcursorLibraryLoadCursor(c->dpy, "grabbing");
	XDefineCursor(c->dpy, w->panel->win, cur);
	XFreeCursor(c->dpy, cur);

	tw->taken = t->win;
}

static void dnd_drag(struct widget *w, struct drag_info *di)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	struct x_connection *c = &w->panel->connection;
	if (tw->dnd_win != None)
		XMoveWindow(c->dpy, tw->dnd_win, di->cur_root_x, di->cur_root_y);
}

static void dnd_drop(struct widget *w, struct drag_info *di)
{
	/* ignore dragged data from other widgets */
	if (di->taken_on != w)
		return;

	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	/* check if we have something draggable */
	if (tw->taken != None) {
		int taken = find_task_by_window(tw, tw->taken);
		int dropped = get_taskbar_task_at(w, di->dropped_x, di->dropped_y);
		if (di->taken_on == di->dropped_on && 
		    taken != -1 && dropped != -1 &&
		    tw->tasks[taken].desktop == tw->tasks[dropped].desktop)
		{
			/* if the desktop is the same.. move task */
			move_task(tw, taken, dropped);
			w->needs_expose = 1;
		} else if (!di->dropped_on && taken != -1) {
			/* out of the panel */
			/* TODO: configurable death threshold */
			if (di->cur_y < -50 || di->cur_y > w->panel->height + 50)
				close_task(c, &tw->tasks[taken]);
		}
	}

	XUndefineCursor(c->dpy, w->panel->win);
	if (tw->dnd_win != None) {
		XDestroyWindow(c->dpy, tw->dnd_win);
		tw->dnd_win = None;
	}
	tw->taken = None;
}
