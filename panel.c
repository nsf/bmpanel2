#include <string.h>
#include <stdio.h>
#include "gui.h"

struct memory_source msrc_panel = MEMSRC(
	"Panel", 
	MEMSRC_DEFAULT_MALLOC, 
	MEMSRC_DEFAULT_FREE, 
	MEMSRC_NO_FLAGS
);

/**************************************************************************
  Panel theme
**************************************************************************/

static int parse_position(const char *pos)
{
	if (strcmp("top", pos) == 0)
		return PANEL_POSITION_TOP;
	else if (strcmp("bottom", pos) == 0)
		return PANEL_POSITION_BOTTOM;
	return PANEL_POSITION_TOP;
}

static bool load_cairo_surface(cairo_surface_t **out, 
		const char *dir, const char *name)
{
	char *file;
	file = xmalloc(strlen(dir) + 1 + strlen(name) + 1, &msrc_panel);
	sprintf(file, "%s/%s", dir, name);

	*out = cairo_image_surface_create_from_png(file);
	xfree(file, &msrc_panel);
	if (cairo_surface_status(*out) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(*out);
		*out = 0;
		return false;
	}
	return true;
}

int panel_theme_load(struct panel_theme *theme, struct theme_format_tree *tree)
{
	memset(theme, 0, sizeof(struct panel_theme));
	struct theme_format_entry *e = theme_format_find_entry(&tree->root, "panel");
	if (!e)
		return 1;

	const char *v;

	theme->position = PANEL_POSITION_TOP; /* default */
	v = theme_format_find_entry_value(e, "position");
	if (v)
		theme->position = parse_position(v);
	
	theme->separator = 0; /* default */
	v = theme_format_find_entry_value(e, "separator");
	if (v) 
		/* ignore error */
		load_cairo_surface(&theme->separator, tree->dir, v);

	/* background is necessary */
	v = theme_format_find_entry_value(e, "background");
	if (!v || !load_cairo_surface(&theme->background, tree->dir, v))
		return 1;

	theme->height = cairo_image_surface_get_height(theme->background);

	return 0;
}

void panel_theme_free(struct panel_theme *theme)
{
	if (theme->background) {
		cairo_surface_destroy(theme->background);
		theme->background = 0;
	}
	if (theme->separator) {
		cairo_surface_destroy(theme->separator);
		theme->separator = 0;
	}
}

/**************************************************************************
  Panel
**************************************************************************/

static void get_position_and_strut(const struct x_connection *c, 
		const struct panel_theme *t, int *ox, int *oy, 
		int *ow, int *oh, long *strut)
{
	int x,y,w,h;
	x = c->workarea_x;
	y = c->workarea_y;
	h = t->height;
	w = c->workarea_width;

	strut[0] = strut[1] = strut[3] = 0;
	strut[2] = h + c->workarea_y;
	if (t->position == PANEL_POSITION_BOTTOM) {
		y = (c->workarea_height + c->workarea_y) - h;
		strut[2] = 0;
		strut[3] = h + c->screen_height - 
			(c->workarea_height + c->workarea_y);
	}

	*ox = x; *oy = y; *oh = h; *ow = w;
}

int panel_create(struct panel *panel, struct theme_format_tree *tree)
{
	if (x_connect(&panel->conn, 0))
		return 1;

	if (panel_theme_load(&panel->theme, tree)) {
		x_disconnect(&panel->conn);
		return 1;
	}

	struct x_connection *c = &panel->conn;
	struct panel_theme *t = &panel->theme;

	int x,y,w,h;
	long strut[4];
	XSetWindowAttributes attrs;
	attrs.background_pixel = 0xFF0000;

	get_position_and_strut(c, t, &x, &y, &w, &h, strut);
	panel->win = XCreateWindow(c->dpy, c->root, x, y, w, h, 0, 
			c->default_depth, InputOutput, 
			c->default_visual, CWBackPixel, &attrs);

	/* NETWM struts */
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_STRUT], 
			XA_CARDINAL, 32, PropModeReplace, (unsigned char*)strut, 4);

	static const struct {
		int s, e;
	} where[] = {
		[PANEL_POSITION_TOP] = {8, 9},
		[PANEL_POSITION_BOTTOM] = {10, 11}
	};

	long strutp[12] = {strut[0], strut[1], strut[2], strut[3],};

	strutp[where[t->position].s] = x;
	strutp[where[t->position].e] = x+w;
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_STRUT_PARTIAL], 
			XA_CARDINAL, 32, PropModeReplace, (unsigned char*)strutp, 12);

	/* desktops */
	long tmp = -1;
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_DESKTOP], 
			XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&tmp, 1);

	/* window type */
	tmp = c->atoms[XATOM_NET_WM_WINDOW_TYPE_DOCK];
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_NET_WM_WINDOW_TYPE], 
			XA_ATOM, 32, PropModeReplace, (unsigned char*)&tmp, 1);

	/* place window on it's position */
	XSizeHints size_hints;

	size_hints.x = x;
	size_hints.y = y;
	size_hints.width = w;
	size_hints.height = h;

	size_hints.flags = PPosition | PMaxSize | PMinSize;
	size_hints.min_width = size_hints.max_width = w;
	size_hints.min_height = size_hints.max_height = h;
	XSetWMNormalHints(c->dpy, panel->win, &size_hints);

	/* motif hints */
	#define MWM_HINTS_DECORATIONS (1L << 1)
	struct mwmhints {
		uint32_t flags;
		uint32_t functions;
		uint32_t decorations;
		int32_t input_mode;
		uint32_t status;
	} mwm = {MWM_HINTS_DECORATIONS,0,0,0,0};
	XChangeProperty(c->dpy, panel->win, c->atoms[XATOM_MOTIF_WM_HINTS], 
			c->atoms[XATOM_MOTIF_WM_HINTS], 32, PropModeReplace, 
			(unsigned char*)&mwm, sizeof(struct mwmhints) / 4);
	#undef MWM_HINTS_DECORATIONS

	/* set classhint */
	XClassHint *ch;
	if ((ch = XAllocClassHint())) {
		ch->res_name = "panel";
		ch->res_class = "bmpanel";
		XSetClassHint(c->dpy, panel->win, ch);
		XFree(ch);
	}

	XMapWindow(c->dpy, panel->win);
	XSync(c->dpy, 0);

	return 0;
}

void panel_destroy(struct panel *panel)
{
	panel_theme_free(&panel->theme);
	x_disconnect(&panel->conn);
}
