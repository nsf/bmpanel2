#include "gui.h"
#include "widget-utils.h"

static void create_win(struct panel *p, int x, int y, 
		       unsigned int w, unsigned int h, long event_mask);
static int create_dc(struct panel *p);
static void blit(struct panel *p, int x, int y, unsigned int w, unsigned int h);
static int create_private(struct panel *p);
static void free_private(struct panel *p);
static void update_bg(struct panel *p);

struct render_interface render_pseudo = {
	.name = "pseudo",
	.create_win = create_win,
	.create_dc = create_dc,
	.blit = blit,
	.create_private = create_private,
	.free_private = free_private,
	.update_bg = update_bg
};

/*
 * blit_cr is a real p->bg interface
 * backbuf is a storage for p->cr
 */
struct pseudo_render {
	Pixmap buf;
	cairo_t *buf_cr;
	cairo_t *blit_cr;
	cairo_surface_t *wallpaper;
};

static int create_private(struct panel *p)
{
	struct x_connection *c = &p->connection;
	struct pseudo_render *pr = xmallocz(sizeof(struct pseudo_render));

	/* TODO: error check */
	pr->blit_cr = create_cairo_for_pixmap(c, p->bg, p->width, p->height);
	pr->wallpaper = cairo_xlib_surface_create(c->dpy, c->root_pixmap,
						  c->default_visual,
						  c->screen_width,
						  c->screen_height);
	pr->buf = x_create_default_pixmap(c, p->width, p->height);
	pr->buf_cr = create_cairo_for_pixmap(c, pr->buf, p->width, p->height);
	p->render_private = (void*)pr;

	return 0;
}

static void free_private(struct panel *p)
{
	Display *dpy = p->connection.dpy;
	struct pseudo_render *pr = p->render_private;
	cairo_destroy(pr->buf_cr);
	cairo_destroy(pr->blit_cr);
	cairo_surface_destroy(pr->wallpaper);
	XFreePixmap(dpy, pr->buf);
	xfree(pr);
}

static void create_win(struct panel *p, int x, int y, 
		       unsigned int w, unsigned int h, long event_mask)
{
	struct x_connection *c = &p->connection;

	p->bg = x_create_default_pixmap(c, w, h);

	/* window */
	XSetWindowAttributes attrs;
	attrs.background_pixmap = p->bg;
	attrs.event_mask = event_mask;

	p->win = x_create_default_window(c, x, y, w, h, 
					 CWBackPixmap | CWEventMask, &attrs);
}

static int create_dc(struct panel *p)
{
	cairo_surface_t *backbuf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
							      p->width, p->height);

	p->cr = cairo_create(backbuf);
	cairo_surface_destroy(backbuf);

	if (cairo_status(p->cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(p->cr);
		return XERROR("Failed to create cairo drawing context");
	}

	cairo_set_operator(p->cr, CAIRO_OPERATOR_SOURCE);

	return 0;
}
	
static void blit(struct panel *p, int x, int y, unsigned int w, unsigned int h)
{
	Display *dpy = p->connection.dpy;
	struct pseudo_render *pr = p->render_private;
	blit_image_ex(pr->wallpaper, pr->buf_cr, p->x + x, p->y + y, w, h, x, y);
	cairo_set_operator(p->cr, CAIRO_OPERATOR_OVER);
	blit_image_ex(cairo_get_target(p->cr), pr->buf_cr, x, y, w, h, x, y);
	cairo_set_operator(p->cr, CAIRO_OPERATOR_SOURCE);
	
	blit_image_ex(cairo_get_target(pr->buf_cr), pr->blit_cr, x, y, w, h, x, y);
	XClearArea(dpy, p->win, x, y, w, h, False);
}

static void update_bg(struct panel *p)
{
	struct x_connection *c = &p->connection;
	struct pseudo_render *pr = p->render_private;

	x_update_root_pmap(c);
	cairo_surface_destroy(pr->wallpaper);
	pr->wallpaper = cairo_xlib_surface_create(c->dpy, c->root_pixmap,
						  c->default_visual,
						  c->screen_width,
						  c->screen_height);
	p->needs_expose = 1;
}
