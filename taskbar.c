#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);
static void prop_change(struct widget *w, XPropertyEvent *e);

static void mouse_enter(struct widget *w);
static void mouse_leave(struct widget *w);

static void dnd_start(struct drag_info *di);
static void dnd_drag(struct drag_info *di);
static void dnd_drop(struct drag_info *di);

static struct widget_interface taskbar_interface = {
	"taskbar",
	WIDGET_SIZE_FILL,
	create_widget_private,
	destroy_widget_private,
	draw,
	button_click,
	0, /* clock_tick */
	prop_change,
	mouse_enter,
	mouse_leave,
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

	if (parse_triple_image(&ts->background, ee, tree)) {
		xwarning("Can't parse taskbar button theme background");
		goto parse_taskbar_state_error_background;
	}

	if (parse_text_info(&ts->font, "font", ee)) {
		xwarning("Can't parse taskbar button theme font");
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
		xwarning("Can't parse 'idle' taskbar button theme");
		goto parse_taskbar_button_theme_error_idle;
	}

	if (parse_taskbar_state(&tt->pressed, "pressed", e, tree)) {
		xwarning("Can't parse 'pressed' taskbar button theme");
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

static void remove_task(struct taskbar_widget *tw, Window win)
{
	int i = find_task_by_window(tw, win);
	if (i == -1)
		xwarning("taskbar: Trying to delete non-existent window");
	else {
		xfree(tw->tasks[i].name);
		if (tw->tasks[i].icon)
			cairo_surface_destroy(tw->tasks[i].icon);
		ARRAY_REMOVE(tw->tasks, i);
	}
}

static void remove_task_i(struct taskbar_widget *tw, size_t i)
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
			remove_task_i(tw, i--);
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

static void draw_task(struct widget *wi, struct taskbar_task *task, int x, int w)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)wi->private;
	cairo_t *cr = wi->panel->cr;


	/* calculations */
	struct triple_image *tbt = (task->win == tw->active) ? 
			&tw->theme.pressed.background :
			&tw->theme.idle.background;
	struct text_info *font = (task->win == tw->active) ?
			&tw->theme.pressed.font :
			&tw->theme.idle.font;
	int leftw = 0;
	int centerw = 0;
	int rightw = 0;
	if (tbt->left)
		leftw = cairo_image_surface_get_width(tbt->left);
	if (tbt->right)
		rightw = cairo_image_surface_get_width(tbt->right);
	centerw = w - leftw - rightw;
	
	int iconw = 0;
	int iconh = 0;
	if (tw->theme.default_icon) {
		iconw = cairo_image_surface_get_width(tw->theme.default_icon);
		iconh = cairo_image_surface_get_height(tw->theme.default_icon);
	}

	int textw = centerw - (iconw + tw->theme.icon_offset[0]);

	/* background */
	int xx = x;
	if (leftw)
		blit_image(tbt->left, cr, xx, wi->y);
	xx += leftw;
	pattern_image(tbt->center, cr, xx, wi->y, centerw);
	xx += centerw;
	if (rightw)
		blit_image(tbt->right, cr, xx, wi->y);
	xx -= centerw;

	/* icon */
	if (iconw && iconh) {
		int yy = wi->y + (wi->height - iconh) / 2;
		xx += tw->theme.icon_offset[0];
		yy += tw->theme.icon_offset[1];
		blit_image(task->icon, cr, xx, yy);
	}
	xx += iconw; 
	
	/* text */
	draw_text(cr, wi->panel->layout, font, task->name, xx, wi->y, textw,
			wi->height);
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

		/* save position for other events */
		t->x = x;
		t->w = taskw;

		draw_task(w, t, x, taskw);
		/* TODO: separators and last button correction */
		x += taskw;
	}
}

static void prop_change(struct widget *w, XPropertyEvent *e)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

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

	int ti = find_task_by_window(tw, e->window);
	if (ti == -1)
		return;

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

int get_taskbar_task_at(struct widget *w, int x, int y, int *side_out)
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

		if (x < (t->x + t->w) && x > t->x &&
		    y > w->y && y < (w->y + w->width)) 
		{
			/* our candidate */
			if (side_out) {
				x -= t->x;
				if (x < t->w / 2)
					*side_out = TASKBAR_TASK_LEFT_SIDE;
				else
					*side_out = TASKBAR_TASK_RIGHT_SIDE;
			}
			return (int)i;			
		}
	}
	return -1;
}

static void activate_task(struct x_connection *c, struct taskbar_task *t)
{
	XClientMessageEvent e;

	e.type = ClientMessage;
	e.window = t->win;
	e.message_type = c->atoms[XATOM_NET_ACTIVE_WINDOW];
	e.format = 32;
	e.data.l[0] = 2;
	e.data.l[1] = CurrentTime;
	e.data.l[2] = 0;
	e.data.l[3] = 0;
	e.data.l[4] = 0;

	XSendEvent(c->dpy, c->root, False, SubstructureNotifyMask |
			SubstructureRedirectMask, (XEvent*)&e);
}

static void button_click(struct widget *w, XButtonEvent *e)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	int ti = get_taskbar_task_at(w, e->x, e->y, 0);
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
}

static void mouse_enter(struct widget *w)
{
}

static void mouse_leave(struct widget *w)
{
}

static void dnd_start(struct drag_info *di)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)di->taken_on->private;
	struct x_connection *c = &di->taken_on->panel->connection;
	struct panel *p = di->taken_on->panel;

	int ti = get_taskbar_task_at(di->taken_on, di->taken_x, di->taken_y, 0);
	if (ti == -1)
		return;

	struct taskbar_task *t = &tw->tasks[ti];
	
	int iconw = 24; 
	int iconh = 24; 
	if (t->icon) {
		iconw = cairo_image_surface_get_width(t->icon);
		iconh = cairo_image_surface_get_width(t->icon);
	}

	XSetWindowAttributes attrs;
	attrs.background_pixel = 0;
	attrs.override_redirect = True;
	
	tw->dnd_win = XCreateWindow(c->dpy, c->root, di->cur_x, di->cur_y, 
			iconw, iconh, 0, CopyFromParent, InputOutput, 
			c->default_visual, CWOverrideRedirect | CWBackPixel, &attrs);
	XMapWindow(c->dpy, tw->dnd_win);
}

static void dnd_drag(struct drag_info *di)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)di->taken_on->private;
	struct x_connection *c = &di->taken_on->panel->connection;
	if (tw->dnd_win != None)
		XMoveWindow(c->dpy, tw->dnd_win, di->cur_x, di->cur_y);
}

static void dnd_drop(struct drag_info *di)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)di->taken_on->private;
	struct x_connection *c = &di->taken_on->panel->connection;
	if (tw->dnd_win != None) {
		XDestroyWindow(c->dpy, tw->dnd_win);
		tw->dnd_win = None;
	}
}
