#ifndef BMPANEL2_GUI_H 
#define BMPANEL2_GUI_H

#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <glib.h>
#include "util.h"
#include "xutil.h"
#include "config-parser.h"

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

	int cur_root_x;
	int cur_root_y;
};

/**************************************************************************
  Widgets
**************************************************************************/

#define WIDGET_SIZE_CONSTANT 1
#define WIDGET_SIZE_FILL 2

struct widget;
struct panel;

/** 
 * Widget interface specification.
 *
 * This struct represents virtual funtions table.
 */
struct widget_interface {
	const char *theme_name;
	int size_type;
	
	int (*create_widget_private)(struct widget *w, struct config_format_entry *e, 
			struct config_format_tree *tree);
	void (*destroy_widget_private)(struct widget *w);
	void (*draw)(struct widget *w);
	void (*button_click)(struct widget *w, XButtonEvent *e);
	void (*clock_tick)(struct widget *w); /* every second */
	void (*prop_change)(struct widget *w, XPropertyEvent *e);
	void (*mouse_enter)(struct widget *w);
	void (*mouse_leave)(struct widget *w);
	void (*mouse_motion)(struct widget *w, XMotionEvent *e);
	void (*configure)(struct widget *w, XConfigureEvent *e);
	void (*client_msg)(struct widget *w, XClientMessageEvent *e);
	void (*win_destroy)(struct widget *w, XDestroyWindowEvent *e);
	
	void (*dnd_start)(struct widget *w, struct drag_info *di);
	void (*dnd_drag)(struct widget *w, struct drag_info *di);
	void (*dnd_drop)(struct widget *w, struct drag_info *di);
};

struct widget {
	struct widget_interface *interface;
	struct panel *panel;

	int x;
	int width;

	int needs_expose;
	int no_separator;

	void *private; /* private part */
};

int register_widget_interface(struct widget_interface *wc);
struct widget_interface *lookup_widget_interface(const char *themename);

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
	/* X stuff */
	Window win;
	Pixmap bg;

	/* widgets */
	size_t widgets_n;
	struct widget widgets[PANEL_MAX_WIDGETS];

	/* "big" things */
	struct panel_theme theme;
	struct x_connection connection;
	cairo_t *cr;
	PangoLayout *layout;
	GMainLoop *loop;

	/* panel dimensions */
	int x;
	int y;
	int width;
	int height;

	/* expose flag */
	int needs_expose;

	/* event dispatching state */
	struct widget *under_mouse;
	struct drag_info dnd;

	struct widget *last_click_widget;
	int last_click_x;
	int last_click_y;
};

int create_panel(struct panel *panel, struct config_format_tree *tree);
void destroy_panel(struct panel *panel);
void panel_main_loop(struct panel *panel);

void recalculate_widgets_sizes(struct panel *panel);

/* event dispatchers */
void disp_button_press_release(struct panel *p, XButtonEvent *e);
void disp_motion_notify(struct panel *p, XMotionEvent *e);
void disp_property_notify(struct panel *p, XPropertyEvent *e);
void disp_enter_leave_notify(struct panel *p, XCrossingEvent *e);
void disp_client_msg(struct panel *p, XClientMessageEvent *e);
void disp_win_destroy(struct panel *p, XDestroyWindowEvent *e);
void disp_configure(struct panel *p, XConfigureEvent *e);

#endif /* BMPANEL2_GUI_H */
