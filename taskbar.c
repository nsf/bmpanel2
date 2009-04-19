#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);

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
	0, /* prop_change */
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
	cairo_surface_destroy(tt->default_icon);
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
		if (tw->tasks[i].desktop == desktop)
			t = (int)i;
	}
	return t;
}

static void add_task(struct taskbar_widget *tw, struct x_connection *c, Window win)
{
	struct taskbar_task t;

	if (x_is_window_hidden(c, win))
		return;

	t.win = win;
	t.name = x_alloc_window_name(c, win); 
	t.icon = get_window_icon(c, win, tw->theme.default_icon);
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
		cairo_surface_destroy(tw->tasks[i].icon);
		ARRAY_REMOVE(tw->tasks, i);
	}
}

static void remove_task_i(struct taskbar_widget *tw, size_t i)
{
	xfree(tw->tasks[i].name);
	cairo_surface_destroy(tw->tasks[i].icon);
	ARRAY_REMOVE(tw->tasks, i);
}

static void free_tasks(struct taskbar_widget *tw)
{
	size_t i;
	for (i = 0; i < tw->tasks_n; ++i) {
		xfree(tw->tasks[i].name);
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
		int delete = 0;
		for (j = 0; j < num; ++j) {
			if (wins[j] == t->win) {
				delete = 1;
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

	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	free_taskbar_theme(&tw->theme);
	free_tasks(tw);
	xfree(tw);
}

static void draw_task(struct widget *w, struct taskbar_task *task, int x, int w)
{

}

static void draw(struct widget *w)
{
	/* I think it's a good idea to calculate all buttons positions here, and
	 * cache these data for later use in other message handlers. User
	 * interacts with what he/she sees, right? 
	 */
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	cairo_t *cr = w->panel->cr;

	int x = w->x;
	blit_image(tw->theme.idle.background.left, cr, x, w->y);
	x += cairo_image_surface_get_width(tw->theme.idle.background.left);
	pattern_image(tw->theme.idle.background.center, cr, x, w->y, 100);
	x += 100;
	blit_image(tw->theme.idle.background.right, cr, x, w->y);
	x += cairo_image_surface_get_width(tw->theme.idle.background.right);

	struct triple_image *tbt = &tw->theme.pressed.background;
	blit_image(tbt->left, cr, x, w->y);
	x += cairo_image_surface_get_width(tbt->left);
	pattern_image(tbt->center, cr, x, w->y, 100);
	x += 100;
	blit_image(tbt->right, cr, x, w->y);
}

static void button_click(struct widget *w, XButtonEvent *e)
{
}

static void mouse_enter(struct widget *w)
{
	printf("taskbar: mouse_enter\n");
}

static void mouse_leave(struct widget *w)
{
	printf("taskbar: mouse_leave\n");
}

static void dnd_start(struct drag_info *di)
{
	printf("taskbar: dnd start! (%d %d)\n", di->taken_x, di->taken_y);
}

static void dnd_drag(struct drag_info *di)
{
	printf("taskbar: dnd drag: %d %d\n", di->cur_x, di->cur_y);
}

static void dnd_drop(struct drag_info *di)
{
	if (!di->dropped_on)
		printf("taskbar: dnd drop (dropped to nowhere)\n");
	else
		printf("taskbar: dnd drop (%s: %d %d)\n", 
			di->dropped_on->interface->theme_name, 
			di->dropped_x, di->dropped_y);
}
