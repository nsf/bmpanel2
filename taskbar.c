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
static int dnd_drop(struct drag_info *di);

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
}

/**************************************************************************
  Taskbar interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree)
{
	struct taskbar_widget *tw = xmallocz(sizeof(struct taskbar_widget));
	tw->pressed = 0;
	if (parse_taskbar_theme(&tw->theme, e, tree)) {
		xfree(tw);
		return -1;
	}
	
	w->private = tw;

	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	free_taskbar_theme(&tw->theme);
	xfree(tw);
}

static void draw(struct widget *w)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	cairo_t *cr = w->panel->cr;
	int x = w->x;
	int i;
	blit_image(tw->theme.idle.background.left, cr, x, w->y);
	x += cairo_image_surface_get_width(tw->theme.idle.background.left);
	pattern_image(tw->theme.idle.background.center, cr, x, w->y, 100);
	x += 100;
	blit_image(tw->theme.idle.background.right, cr, x, w->y);
	x += cairo_image_surface_get_width(tw->theme.idle.background.right);

	struct triple_image *tbt = (tw->pressed) ? &tw->theme.pressed.background :
						&tw->theme.idle.background;
	blit_image(tbt->left, cr, x, w->y);
	x += cairo_image_surface_get_width(tbt->left);
	pattern_image(tbt->center, cr, x, w->y, 100);
	x += 100;
	blit_image(tbt->right, cr, x, w->y);
}

static void button_click(struct widget *w, XButtonEvent *e)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	if (e->type == ButtonPress)
		tw->pressed = 1;
	else
		tw->pressed = 0;
	w->needs_expose = 1;
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

static int dnd_drop(struct drag_info *di)
{
	if (!di->dropped_on)
		printf("taskbar: dnd drop (dropped to nowhere)\n");
	else
		printf("taskbar: dnd drop (%s: %d %d)\n", 
			di->dropped_on->interface->theme_name, 
			di->dropped_x, di->dropped_y);
	return 0;
}
