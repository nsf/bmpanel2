#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
		struct config_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);
static void prop_change(struct widget *w, XPropertyEvent *e);

static void dnd_drop(struct widget *w, struct drag_info *di);

static struct widget_interface desktops_interface = {
	"desktop_switcher",
	WIDGET_SIZE_CONSTANT,
	create_widget_private,
	destroy_widget_private,
	draw,
	button_click,
	0, /* clock_tick */
	prop_change,
	0, /* mouse_enter */
	0, /* mouse_leave */
	0, /* mouse_motion */
	0, /* dnd_start */
	0, /* dnd_drag */
	dnd_drop
};

void register_desktop_switcher()
{
	register_widget_interface(&desktops_interface);
}

/**************************************************************************
  Desktop switcher theme
**************************************************************************/

static int parse_desktops_state(struct desktops_state *ds, const char *name,
				struct config_format_entry *e,
				struct config_format_tree *tree)
{
	struct config_format_entry *ee = find_config_format_entry(e, name);
	if (!ee) {
		required_entry_not_found(e, name);
		return -1;
	}

	if (parse_triple_image(&ds->background, ee, tree, 1))
		goto parse_desktops_state_error_background;

	if (parse_text_info_named(&ds->font, "font", ee, 1))
		goto parse_desktops_state_error_font;

	ds->left_corner = parse_image_part_named("left_corner", ee, tree, 0);
	ds->right_corner = parse_image_part_named("right_corner", ee, tree, 0);

	return 0;

parse_desktops_state_error_font:
	free_triple_image(&ds->background);
parse_desktops_state_error_background:
	return -1;
}

static void free_desktops_state(struct desktops_state *ds)
{
	free_triple_image(&ds->background);
	free_text_info(&ds->font);
	if (ds->left_corner)
		cairo_surface_destroy(ds->left_corner);
	if (ds->right_corner)
		cairo_surface_destroy(ds->right_corner);
}

static int parse_desktops_theme(struct desktops_theme *dt,
				struct config_format_entry *e,
				struct config_format_tree *tree)
{
	if (parse_desktops_state(&dt->idle, "idle", e, tree))
		goto parse_desktops_state_error_idle;

	if (parse_desktops_state(&dt->pressed, "pressed", e, tree))
		goto parse_desktops_state_error_pressed;
	
	dt->separator = parse_image_part_named("separator", e, tree, 0);

	return 0;

parse_desktops_state_error_pressed:
	free_desktops_state(&dt->idle);
parse_desktops_state_error_idle:
	return -1;
}

static void free_desktops_theme(struct desktops_theme *dt)
{
	free_desktops_state(&dt->idle);
	free_desktops_state(&dt->pressed);
	if (dt->separator)
		cairo_surface_destroy(dt->separator);
}
