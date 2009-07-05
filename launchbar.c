#include "builtin-widgets.h"
#include "settings.h"

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
				 struct config_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void mouse_motion(struct widget *w, XMotionEvent *e);
static void mouse_leave(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);

struct widget_interface launchbar_interface = {
	.theme_name		= "launchbar",
	.size_type		= WIDGET_SIZE_CONSTANT,
	.create_widget_private	= create_widget_private,
	.destroy_widget_private = destroy_widget_private,
	.draw			= draw,
	.mouse_motion		= mouse_motion,
	.mouse_leave		= mouse_leave,
	.button_click		= button_click
};

static int get_item(struct launchbar_widget *lw, int x, int y)
{
	size_t i;
	for (i = 0; i < lw->items_n; ++i) {
		struct launchbar_item *item = &lw->items[i];
		if (   x >= item->x && x < (item->x + item->w)
		    && y >= item->y && y < (item->y + item->h) )
				return (int)i;
	}
	return -1;
}

void parse_items(struct launchbar_widget *lw)
{
	size_t i;
	struct config_format_entry *e = find_config_format_entry(&g_settings.root, 
								 "launchbar");
	if (!e)
		return;

	ENSURE_ARRAY_CAPACITY(lw->items, e->children_n);
	for (i = 0; i < e->children_n; ++i) {
		struct launchbar_item lbitem;
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

		lbitem.icon = copy_resized(icon, lw->icon_w, lw->icon_h);
		cairo_surface_destroy(icon);
		lbitem.execstr = xstrdup(ee->value);

		ARRAY_APPEND(lw->items, lbitem);
	}
}

static void update_launchbar_size(struct widget *w)
{
	struct launchbar_widget *sw = (struct launchbar_widget*)w->private;
	
	if (w->panel->theme.vertical) {
		w->width = w->panel->width;
		sw->icon_row_len = w->width / (sw->icon_w + sw->padding_x);
		if ( sw->icon_row_len < 1 )
			sw->icon_row_len = 1;
		sw->icon_rows = (sw->items_n + sw->icon_row_len - 1) / sw->icon_row_len; /* ceil rounding */
		w->height = sw->icon_rows * (sw->icon_h + sw->padding_y);
	}
	else {
		w->height = w->panel->height;
		sw->icon_rows = w->height / (sw->icon_h + sw->padding_y);
		if ( sw->icon_rows < 1 )
			sw->icon_rows = 1;
		sw->icon_row_len = (sw->items_n + sw->icon_rows - 1) / sw->icon_rows; /* ceil rounding */
		w->width = sw->icon_row_len * (sw->icon_w + sw->padding_x);
	}
}

/**************************************************************************
  Launch Bar interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
				 struct config_format_tree *tree)
{
	struct launchbar_widget *lw = xmallocz(sizeof(struct launchbar_widget));
	parse_2ints(&lw->icon_w, &lw->icon_h, "icon_size", e);
	parse_2ints(&lw->padding_x, &lw->padding_y, "padding", e);

	INIT_EMPTY_ARRAY(lw->items);
	lw->active = -1;
	w->private = lw;
	parse_items(lw);
	update_launchbar_size(w);
	
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
	xfree(lw);
}

static void draw(struct widget *w)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;
	cairo_t *cr = w->panel->cr;

	size_t row, j;
	
	int y = w->y + (w->height - lw->icon_h * lw->icon_rows) / 2;
	for (row = 0; row < lw->icon_rows; ++row) {
		int x = w->x + (w->width - lw->icon_w * lw->icon_row_len) / 2;
		for (j = 0; j < lw->icon_row_len; ++j) {
			size_t i = row * lw->icon_row_len + j;
			if ( i >= lw->items_n )
				break;
			
			lw->items[i].x = x;
			lw->items[i].y = y;
			lw->items[i].w = lw->icon_w;
			lw->items[i].h = lw->icon_h;
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			blit_image(lw->items[i].icon, cr, x, y);
			if (i == lw->active) {
				cairo_save(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
				cairo_set_source_rgba(cr, 1, 1, 1, 0.2);
				cairo_rectangle(cr, x, y, 
						lw->icon_w, lw->icon_h);
				cairo_clip(cr);
				cairo_mask_surface(cr, lw->items[i].icon, x, y);
				cairo_restore(cr);
			}
			cairo_restore(cr);
			x += lw->icon_w + lw->padding_x;
		}
		y += lw->icon_h + lw->padding_y;
	}
}

static void mouse_motion(struct widget *w, XMotionEvent *e)
{
	struct launchbar_widget *lw = (struct launchbar_widget*)w->private;
	int cur = get_item(lw, e->x, e->y);
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
	int cur = get_item(lw, e->x, e->y);
	if (cur == -1)
		return;

	if (e->button == 1 && e->type == ButtonRelease)
		g_spawn_command_line_async(lw->items[cur].execstr, 0);
}
