#ifndef BMPANEL2_GUI_H 
#define BMPANEL2_GUI_H

#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <glib.h>
#include "util.h"
#include "xutil.h"
#include "theme-parser.h"

/**************************************************************************
  Image cache
**************************************************************************/

/* surfaces are referenced, should be released with "cairo_surface_destroy" */
cairo_surface_t *get_image(const char *path);
cairo_surface_t *get_image_part(const char *path, int x, int y, int w, int h);
void clean_image_cache();

/**************************************************************************
  Drag'n'drop
**************************************************************************/

struct drag_info {
	struct widget *taken_on;
	int taken_x;
	int taken_y;

	struct widget *dropped_on;
	int dropped_x;
	int dropped_y;

	int cur_x;
	int cur_y;
};

/**************************************************************************
  Widgets
**************************************************************************/

#define WIDGET_SIZE_CONSTANT 1
#define WIDGET_SIZE_FILL 2

struct widget;
struct panel;

struct widget_interface {
	const char *theme_name;
	int size_type;
	
	int (*create_widget_private)(struct widget *w, struct theme_format_entry *e, 
			struct theme_format_tree *tree);
	void (*destroy_widget_private)(struct widget *w);
	void (*draw)(struct widget *w);
	void (*button_click)(struct widget *w, XButtonEvent *e);
	void (*clock_tick)(struct widget *w); /* every second */
	void (*prop_change)(struct widget *w, XPropertyEvent *e);
};

struct widget {
	struct widget_interface *interface;
	struct panel *panel;

	/* rectangle for event dispatching */
	int x;
	int y;
	int width;
	int height;

	int needs_expose;

	void *private; /* private part */
};

int register_widget_interface(struct widget_interface *wc);
struct widget_interface *lookup_widget_interface(const char *themename);

void register_taskbar();
void register_desktop_switcher();
void register_systray();
void register_clock();

/**************************************************************************
  Panel
**************************************************************************/

#define PANEL_POSITION_TOP 0
#define PANEL_POSITION_BOTTOM 1

struct panel_theme {
	int position;
	cairo_surface_t *background;
	cairo_surface_t *separator;
};

#define PANEL_MAX_WIDGETS 20

struct panel {
	Window win;
	Pixmap bg;

	size_t widgets_n;
	struct widget widgets[PANEL_MAX_WIDGETS];

	struct panel_theme theme;
	struct x_connection connection;
	cairo_t *cr;
	PangoLayout *layout;

	GMainLoop *loop;

	int x;
	int y;
	int width;
	int height;
};

int create_panel(struct panel *panel, struct theme_format_tree *tree);
void destroy_panel(struct panel *panel);
void panel_main_loop(struct panel *panel);

#endif /* BMPANEL2_GUI_H */
