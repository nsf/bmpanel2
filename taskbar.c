#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);

static struct widget_interface taskbar_interface = {
	"taskbar",
	WIDGET_SIZE_FILL,
	create_widget_private,
	destroy_widget_private,
	draw,
	button_click,
	0, /* clock_tick */
	0, /* prop_change */
};

void register_taskbar()
{
	register_widget_interface(&taskbar_interface);
}

/**************************************************************************
  Taskbar theme
**************************************************************************/

static int parse_taskbar_theme(struct taskbar_theme *tt, 
		struct theme_format_entry *e, struct theme_format_tree *tree)
{
	if (parse_triple_image(&tt->idle, "idle", e, tree)) {
		xwarning("Can't parse 'idle' taskbar button theme");
		goto parse_taskbar_button_theme_error_idle;
	}

	if (parse_triple_image(&tt->pressed, "pressed", e, tree)) {
		xwarning("Can't parse 'pressed' taskbar button theme");
		goto parse_taskbar_button_theme_error_pressed;
	}

	return 0;

parse_taskbar_button_theme_error_pressed:
	free_triple_image(&tt->idle);
parse_taskbar_button_theme_error_idle:
	return -1;
}

static void free_taskbar_theme(struct taskbar_theme *tt)
{
	free_triple_image(&tt->idle);
	free_triple_image(&tt->pressed);
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
	blit_image(tw->theme.idle.left, cr, x, w->y);
	x += cairo_image_surface_get_width(tw->theme.idle.left);
	pattern_image(tw->theme.idle.center, cr, x, w->y, 100);
	x += 100;
	blit_image(tw->theme.idle.right, cr, x, w->y);
	x += cairo_image_surface_get_width(tw->theme.idle.right);

	struct triple_image *tbt = (tw->pressed) ? &tw->theme.pressed :
						&tw->theme.idle;
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
	//g_main_quit(w->panel->loop);
}
