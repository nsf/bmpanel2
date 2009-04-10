#include "drawing-utils.h"
#include "parsing-utils.h"
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
};

void register_taskbar()
{
	register_widget_interface(&taskbar_interface);
}

/**************************************************************************
  Taskbar theme
**************************************************************************/

static int parse_taskbar_button_theme(struct taskbar_button_theme *tbt,
		const char *name, struct theme_format_entry *e, 
		struct theme_format_tree *tree)
{
	struct theme_format_entry *ee = find_theme_format_entry(e, name);
	if (!ee)
		return -1;

	tbt->center = parse_image_part_named("center", ee, tree);
	if (!tbt->center)
		return xerror("Can't parse 'center' image of taskbar button theme");

	tbt->left = parse_image_part_named("left", ee, tree);
	tbt->right = parse_image_part_named("right", ee, tree);

	return 0;
}

static void free_taskbar_button_theme(struct taskbar_button_theme *tbt)
{
	cairo_surface_destroy(tbt->center);
	if (tbt->left) 
		cairo_surface_destroy(tbt->left);
	if (tbt->right) 
		cairo_surface_destroy(tbt->right);
}

static int parse_taskbar_theme(struct taskbar_theme *tt, 
		struct theme_format_entry *e, struct theme_format_tree *tree)
{
	if (parse_taskbar_button_theme(&tt->idle, "idle", e, tree)) {
		xwarning("Can't parse 'idle' taskbar button theme");
		goto parse_taskbar_button_theme_error_idle;
	}

	if (parse_taskbar_button_theme(&tt->pressed, "pressed", e, tree)) {
		xwarning("Can't parse 'pressed' taskbar button theme");
		goto parse_taskbar_button_theme_error_pressed;
	}

	return 0;

parse_taskbar_button_theme_error_pressed:
	free_taskbar_button_theme(&tt->idle);
parse_taskbar_button_theme_error_idle:
	return -1;
}

static void free_taskbar_theme(struct taskbar_theme *tt)
{
	free_taskbar_button_theme(&tt->idle);
	free_taskbar_button_theme(&tt->pressed);
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
	
	w->private = tw;

	return 0;
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

	blit_image(tw->theme.pressed.left, cr, x, w->y);
	x += cairo_image_surface_get_width(tw->theme.pressed.left);
	pattern_image(tw->theme.pressed.center, cr, x, w->y, 100);
	x += 100;
	blit_image(tw->theme.pressed.right, cr, x, w->y);
}

static void destroy_widget_private(struct widget *w)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w->private;
	free_taskbar_theme(&tw->theme);
	xfree(tw);
}

static void button_click(struct widget *w, XButtonEvent *e)
{
	if (e->type == ButtonPress)
		printf("taskbar button pressed\n");
	else
		printf("taskbar button released\n");
	draw(w);
	printf("%d %d %d %d\n", w->x, w->y, w->width, w->height);
	expose_panel(w->panel, w->x, w->y, w->width, w->height);

	g_main_quit(w->panel->loop);
}
