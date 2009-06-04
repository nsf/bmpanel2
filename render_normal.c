#include "gui.h"
#include "widget-utils.h"

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
	p->cr = create_cairo_for_pixmap(&p->connection, p->bg, 
					p->width, p->height);
	if (!p->cr)
		return XERROR("Failed to create cairo drawing context");

	return 0;
}
	
static void blit(struct panel *p, int x, int y, unsigned int w, unsigned int h)
{
	XClearArea(p->connection.dpy, p->win, x, y, w, h, False);
}
