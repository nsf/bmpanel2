#include "gui.h"

static void create_win(struct panel *p, int x, int y, 
		       unsigned int w, unsigned int h, long event_mask);
static int create_dc(struct panel *p);
static void blit(struct panel *p, int x, int y, unsigned int w, unsigned int h);

struct render_interface render_normal = {
	.name = "normal",
	.create_win = create_win,
	.create_dc = create_dc,
	.blit = blit
};

static void create_win(struct panel *p, int x, int y, 
		       unsigned int w, unsigned int h, long event_mask)
{
	struct x_connection *c = &p->connection;

	p->bg = x_create_default_pixmap(c, w, h);

	XSetWindowAttributes attrs;
	attrs.background_pixmap = p->bg;
	attrs.event_mask = event_mask;

	p->win = x_create_default_window(c, x, y, w, h, 
					 CWBackPixmap | CWEventMask, &attrs);
}

static int create_dc(struct panel *p)
{
	struct x_connection *c = &p->connection;

	cairo_surface_t *bgs = cairo_xlib_surface_create(c->dpy, 
			p->bg, c->default_visual, 
			p->width, p->height);

	if (cairo_surface_status(bgs) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(bgs);
		return XERROR("Failed to create cairo surface");
	}

	p->cr = cairo_create(bgs);
	cairo_surface_destroy(bgs);

	if (cairo_status(p->cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(p->cr);
		return XERROR("Failed to create cairo drawing context");
	}

	return 0;

}
	
static void blit(struct panel *p, int x, int y, unsigned int w, unsigned int h)
{
	Display *dpy = p->connection.dpy;
	XClearArea(dpy, p->win, x, y, w, h, False);
}
