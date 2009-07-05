#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
		struct config_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void button_click(struct widget *w, XButtonEvent *e);
static void prop_change(struct widget *w, XPropertyEvent *e);
static void client_msg(struct widget *w, XClientMessageEvent *e);

static void dnd_drop(struct widget *w, struct drag_info *di);

struct widget_interface desktops_interface = {
	.theme_name 		= "desktop_switcher",
	.size_type 		= WIDGET_SIZE_CONSTANT,
	.create_widget_private 	= create_widget_private,
	.destroy_widget_private = destroy_widget_private,
	.draw 			= draw,
	.button_click 		= button_click,
	.prop_change 		= prop_change,
	.dnd_drop 		= dnd_drop,
	.client_msg		= client_msg
};

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

/**************************************************************************
  Desktops management
**************************************************************************/

static void free_desktops(struct desktops_widget *dw)
{
	size_t i;
	for (i = 0; i < dw->desktops_n; ++i)
		xfree(dw->desktops[i].name);
	CLEAR_ARRAY(dw->desktops);
}

static void update_active_desktop(struct desktops_widget *dw, struct x_connection *c)
{
	dw->active = x_get_prop_int(c, c->root, 
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

static void update_desktops(struct desktops_widget *dw, struct x_connection *c)
{
	free_desktops(dw);
	update_active_desktop(dw, c);
	int desktops_n = x_get_prop_int(c, c->root, 
					c->atoms[XATOM_NET_NUMBER_OF_DESKTOPS]);
	char *name, *names;
	size_t i;
	names = name = x_get_prop_data(c, c->root, 
				       c->atoms[XATOM_NET_DESKTOP_NAMES],
				       c->atoms[XATOM_UTF8_STRING], 0);

	for (i = 0; i < desktops_n; ++i) {
		struct desktops_desktop d = {0,0,0,0};
		if (names) {
			d.name = xstrdup(name);
			name += strlen(name) + 1;
		} else {
			char buf[16];
			snprintf(buf, sizeof(buf), "%u", i+1);
			d.name = xstrdup(buf);
		}
		ARRAY_APPEND(dw->desktops, d);
	}

	if (names)
		XFree(names);
}

static void resize_desktops(struct widget *w)
{
	struct desktops_widget *dw = (struct desktops_widget*)w->private;
	struct desktops_state *ds = &dw->theme.idle;
	size_t i;

	int left_cornerw = image_width(ds->left_corner);
	int leftw = image_width(ds->background.left);
	int rightw = image_width(ds->background.right);
	int right_cornerw = image_width(ds->right_corner);
	int sepw = image_width(dw->theme.separator);

	int width = 0;
	for (i = 0; i < dw->desktops_n; ++i) {
		int text_w = 0;
		int this_w = 0;
		
		dw->desktops[i].x = w->x + width;
		
		if (i == 0)
			this_w += left_cornerw;
		else 
			this_w += leftw;
		
		text_extents(w->panel->layout, ds->font.pfd, 
			     dw->desktops[i].name, &text_w, 0);
		dw->desktops[i].textw = text_w;
		this_w += text_w;
		
		if (i == dw->desktops_n - 1)
			this_w += right_cornerw;
		else 
			this_w += rightw;
		
		dw->desktops[i].w = this_w;
		
		width += this_w + sepw;
	}
	w->width = width;
	w->height = w->panel->height;
}

static int get_desktop_at(struct widget *w, int x, int y)
{
	struct desktops_widget *dw = (struct desktops_widget*)w->private;

	size_t i;
	for (i = 0; i < dw->desktops_n; ++i) {
		struct desktops_desktop *d = &dw->desktops[i];
		if (   x >= d->x && x < (d->x + d->w)
		    && y >= d->y && y < (d->y + d->w) )
				return (int)i;
	}
	return -1;
}

/**************************************************************************
  Desktop switcher interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
		struct config_format_tree *tree)
{
	struct desktops_widget *dw = xmallocz(sizeof(struct desktops_widget));
	if (parse_desktops_theme(&dw->theme, e, tree)) {
		xfree(dw);
		XWARNING("Failed to parse desktop switcher theme");
		return -1;
	}

	INIT_ARRAY(dw->desktops, 16);
	w->private = dw;

	struct x_connection *c = &w->panel->connection;
	update_desktops(dw, c);
	resize_desktops(w);

	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct desktops_widget *dw = (struct desktops_widget*)w->private;
	free_desktops_theme(&dw->theme);
	free_desktops(dw);
	FREE_ARRAY(dw->desktops);
	xfree(dw);
}

static void draw(struct widget *w)
{
	struct desktops_widget *dw = (struct desktops_widget*)w->private;
	struct desktops_state *idle = &dw->theme.idle;
	struct desktops_state *pressed = &dw->theme.pressed;
	cairo_t *cr = w->panel->cr;
	size_t i;

	int x = w->x;
	int y = w->y;
	
	int left_cornerw = image_width(idle->left_corner);
	int leftw = image_width(idle->background.left);
	int rightw = image_width(idle->background.right);
	int right_cornerw = image_width(idle->right_corner);
	int sepw = image_width(dw->theme.separator);
	int h = w->height;

	for (i = 0; i < dw->desktops_n; ++i) {
		struct desktops_state *cur = (i == dw->active) ? pressed : idle;
		
		if (i == 0 && left_cornerw) {
			blit_image(cur->left_corner, cr, x, y); 
			x += left_cornerw;
		} else if (leftw) {
			blit_image(cur->background.left, cr, x, y);
			x += leftw;
		}
		int width = dw->desktops[i].textw;

		pattern_image(cur->background.center, cr, x, y, width, h);
		draw_text(cr, w->panel->layout, &cur->font, dw->desktops[i].name,
			  x, 0, width, h);

		x += width;
		if (i == dw->desktops_n - 1 && right_cornerw) {
			blit_image(cur->right_corner, cr, x, y);
			x += right_cornerw;
		} else {
			if (rightw) {
				blit_image(cur->background.right, cr, x, y);
				x += rightw;
			}
			if (sepw) {
				blit_image(dw->theme.separator, cr, x, y);
				x += sepw;
			}
		}
	}
}

static void button_click(struct widget *w, XButtonEvent *e)
{
	struct desktops_widget *dw = (struct desktops_widget*)w->private;
	int di = get_desktop_at(w, e->x, e->y);
	if (di == -1)
		return;
	
	struct x_connection *c = &w->panel->connection;

	if (e->button == 1 && e->type == ButtonRelease && dw->active != di)
		switch_desktop(di, c);
}

static void prop_change(struct widget *w, XPropertyEvent *e)
{
	struct desktops_widget *dw = (struct desktops_widget*)w->private;
	struct x_connection *c = &w->panel->connection;

	if (e->window == c->root) {
		if (e->atom == c->atoms[XATOM_NET_NUMBER_OF_DESKTOPS] ||
		    e->atom == c->atoms[XATOM_NET_DESKTOP_NAMES])
		{
			update_desktops(dw, c);
			resize_desktops(w);
			recalculate_widgets_sizes(w->panel);
			return;
		}

		if (e->atom == c->atoms[XATOM_NET_CURRENT_DESKTOP]) {
			update_active_desktop(dw, c);
			w->needs_expose = 1;
			return;
		}
	}
}

static void client_msg(struct widget *w, XClientMessageEvent *e)
{
	struct panel *p = w->panel;
	struct x_connection *c = &p->connection;
	struct desktops_widget *dw = (struct desktops_widget*)w->private;

	if (e->message_type == c->atoms[XATOM_XDND_POSITION]) {
		int x = (e->data.l[2] >> 16) & 0xFFFF;
		int y = e->data.l[2] & 0xFFFF;

		/* if it's not ours, skip.. */
		if (   x >= (p->x + w->x) && x < (p->x + w->x + w->width)
		    && y >= (p->y + w->y) && y < (p->y + w->y + w->height) )
		{
			int di = get_desktop_at(w, x - p->x, y - p->y);
			if (di != -1 && di != dw->active)
					switch_desktop(di, c);

			x_send_dnd_message(c, e->data.l[0], 
					c->atoms[XATOM_XDND_STATUS],
					p->win,
					2, /* bits: 0 1 */
					0, 0, 
					None);
		}
	}
}

static void dnd_drop(struct widget *w, struct drag_info *di)
{
	if (di->taken_on->interface != &taskbar_interface)
		return;

	struct taskbar_widget *tw = di->taken_on->private;
	if (tw->taken == None)
		return;

	int desktop = get_desktop_at(w, di->dropped_x, di->dropped_y);
	if (desktop == -1)
		return;

	struct x_connection *c = &w->panel->connection;
	x_send_netwm_message(c, tw->taken, 
			     c->atoms[XATOM_NET_WM_DESKTOP],
			     (long)desktop, 2, 0, 0, 0);
}
