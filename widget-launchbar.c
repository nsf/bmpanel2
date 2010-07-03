#include "settings.h"
#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct config_format_entry *e,
				 struct config_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void mouse_motion(struct widget *w, XMotionEvent *e);
static void mouse_leave(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);
static void reconfigure(struct widget *w);

struct widget_interface launchbar_interface = {
	.theme_name		= "launchbar",
	.size_type		= WIDGET_SIZE_CONSTANT,
	.create_widget_private	= create_widget_private,
	.destroy_widget_private = destroy_widget_private,
	.draw			= draw,
	.mouse_motion		= mouse_motion,
	.mouse_leave		= mouse_leave,
	.button_click		= button_click,
	.reconfigure		= reconfigure
};

static int get_item(struct launchbar_widget *lw, int x)
{
	size_t i;
	for (i = 0; i < lw->items_n; ++i) {
		struct launchbar_item *item = &lw->items[i];
		if (item->x < x && (item->x + item->w) > x)
			return (int)i;
	}
	return -1;
}

static int parse_items(struct launchbar_widget *lw)
{
	int items = 0;
	size_t i;
	struct config_format_entry *e = find_config_format_entry(&g_settings.root,
								 "launchbar");
	if (!e)
		return 0;

	ENSURE_ARRAY_CAPACITY(lw->items, e->children_n);
	for (i = 0; i < e->children_n; ++i) {
		struct launchbar_item lbitem = {0,0,0,0};
		struct config_format_entry *ee = &e->children[i];

		/* if no exec path, skip */
		if (!ee->value)
			continue;

		/* if there is no icon, skip */
		const char *icon_path = find_config_format_entry_value(ee, "icon");
		if (!icon_path)
			continue;

		/* if can't load icon, skip */
		cairo_surface_t *icon = get_image(icon_path);
		if (!icon)
			continue;

		lbitem.icon = copy_resized(icon,
					   lw->theme.icon_size[0],
					   lw->theme.icon_size[1]);
		cairo_surface_destroy(icon);
		lbitem.execstr = xstrdup(ee->value);

		ARRAY_APPEND(lw->items, lbitem);
		items++;
	}

	return items;
}

static int parse_launchbar_theme(struct launchbar_theme *theme,
				 struct config_format_entry *e,
				 struct config_format_tree *tree)
{
	if (parse_2ints(theme->icon_size, "icon_size", e) != 0)
		return -1;
	parse_triple_image_named(&theme->background, "background", e, tree, 0);
	parse_2ints(theme->icon_offset, "icon_offset", e);
	theme->icon_spacing = parse_int("icon_spacing", e, 0);
	return 0;
}

static void free_launchbar_theme(struct launchbar_theme *theme)
{
	free_triple_image(&theme->background);
}

static int calc_width(struct launchbar_theme *lt,
		      int items)
{
	return items * (lt->icon_size[0] + lt->icon_spacing) -
		lt->icon_spacing +
		image_width(lt->background.left) +
		image_width(lt->background.right);
}

/**************************************************************************
  Launch Bar interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct config_format_entry *e,
				 struct config_format_tree *tree)
{
	struct launchbar_widget *lw = xmallocz(sizeof(struct launchbar_widget));
	struct launchbar_theme *lt = &lw->theme;
	if (parse_launchbar_theme(lt, e, tree)) {
		xfree(lw);
		XWARNING("Failed to parse launchbar theme");
		return -1;
	}

	INIT_EMPTY_ARRAY(lw->items);
	lw->active = -1;
	int items = parse_items(lw);

	w->width = calc_width(lt, items);
	w->private = lw;

	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;
	size_t i;
	for (i = 0; i < lw->items_n; ++i) {
		cairo_surface_destroy(lw->items[i].icon);
		xfree(lw->items[i].execstr);
	}
	FREE_ARRAY(lw->items);
	free_launchbar_theme(&lw->theme);
	xfree(lw);
}

static void reconfigure(struct widget *w)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;

	/* free items */
	size_t i;
	for (i = 0; i < lw->items_n; ++i) {
		cairo_surface_destroy(lw->items[i].icon);
		xfree(lw->items[i].execstr);
	}
	CLEAR_ARRAY(lw->items);
	lw->active = -1;
	int items = parse_items(lw);

	w->width = calc_width(&lw->theme, items);
}

static void draw(struct widget *w)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;
	struct launchbar_theme *lt = &lw->theme;
	size_t i;

	cairo_t *cr = w->panel->cr;
	int x = w->x;
	int leftw = 0;
	int rightw = 0;
	int centerw = w->width;
	if (lt->background.center) {
		leftw += image_width(lt->background.left);
		rightw += image_width(lt->background.right);
		centerw -= leftw + rightw;

		/* left */
		if (leftw)
			blit_image(lt->background.left, cr, x, 0);
		x += leftw;

		/* center */
		pattern_image(lt->background.center, cr, x, 0, centerw, 1);
		x += centerw;

		/* right */
		if (rightw)
			blit_image(lt->background.right, cr, x, 0);
		x -= centerw;
	}
	x = w->x + leftw + lt->icon_offset[0];
	int y = (w->panel->height - lt->icon_size[1]) / 2 + lt->icon_offset[1];
	for (i = 0; i < lw->items_n; ++i) {
		lw->items[i].x = x;
		lw->items[i].w = lt->icon_size[0];
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		blit_image(lw->items[i].icon, cr, x, y);
		if (i == lw->active) {
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
			cairo_set_source_rgba(cr, 1, 1, 1, 0.2);
			cairo_rectangle(cr, x, y,
					lt->icon_size[0], lt->icon_size[1]);
			cairo_clip(cr);
			cairo_mask_surface(cr, lw->items[i].icon, x, y);
			cairo_restore(cr);
		}
		cairo_restore(cr);
		x += lt->icon_size[0] + lt->icon_spacing;
	}
}

static void mouse_motion(struct widget *w, XMotionEvent *e)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;
	int cur = get_item(lw, e->x);
	if (cur != lw->active) {
		lw->active = cur;
		w->needs_expose = 1;
	}
}

static void mouse_leave(struct widget *w)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;
	if (lw->active != -1) {
		lw->active = -1;
		w->needs_expose = 1;
	}
}

static void button_click(struct widget *w, XButtonEvent *e)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;
	int cur = get_item(lw, e->x);
	if (cur == -1)
		return;

	int mbutton_use = check_mbutton_condition(w->panel, e->button, MBUTTON_USE);
	if (mbutton_use && e->type == ButtonRelease)
		g_spawn_command_line_async(lw->items[cur].execstr, 0);
}
