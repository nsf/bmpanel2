#pragma once

#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <glib.h>
#include "util.h"
#include "xutil.h"
#include "config-parser.h"

#define MININT(a, b) ({int _a = (a), _b = (b); _a < _b ? _a : _b; })
#define MAXINT(a, b) ({int _a = (a), _b = (b); _a > _b ? _a : _b; })

/**************************************************************************
  Mouse buttons
**************************************************************************/

#define MBUTTON_USE		(1<<0)
#define MBUTTON_DRAG		(1<<1)
#define MBUTTON_KILL		(1<<2)
#define MBUTTON_SHOW_DESKTOP	(1<<3)

#define MBUTTON_1_DEFAULT	(MBUTTON_USE | MBUTTON_DRAG)
#define MBUTTON_2_DEFAULT	(MBUTTON_KILL)
#define MBUTTON_3_DEFAULT	0

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

	int button;

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

	/* this is a hack, but it is required for pseudo-transparency */
	void (*panel_exposed)(struct widget *w);
	void (*reconfigure)(struct widget *w);
	int (*retheme_reconfigure)(struct widget *w, struct config_format_entry *e,
				   struct config_format_tree *tree);
};

struct widget {
	struct widget_interface *interface;
	struct panel *panel;

	int x;
	int width;

	int needs_expose;
	int no_separator;
	int paint_replace; /* for transparent render */

	void *private; /* private part */
};

struct widget_interface *lookup_widget_interface(const char *themename);

/* alternatives */
void update_alternatives_preference(char *prefstr, struct config_format_tree *tree);
int validate_widget_for_alternatives(const char *theme_name);
void reset_alternatives();

/**************************************************************************
  Panel
**************************************************************************/

struct widget_stash {
	struct widget *widgets;
	size_t widgets_n;
};

#define PANEL_POSITION_TOP 0
#define PANEL_POSITION_BOTTOM 1

struct panel_theme {
	int position;
	cairo_surface_t *background;
	cairo_surface_t *separator;
	int transparent; /* bool */
	int align;
	int width;
	int height;
	int width_in_percents; /* bool */
};

#define PANEL_MAX_WIDGETS 20

struct render_interface;

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

	int monitor;

	int showing_desktop;

	/* expose flag */
	int needs_expose;

	/* event dispatching state */
	int drag_threshold;

	struct widget *under_mouse;
	struct drag_info dnd;

	struct widget *last_click_widget;
	int last_click_x;
	int last_click_y;
	int last_button;

	/* binded mouse actions */
	unsigned int mbutton[3];

	/* render interface */
	struct render_interface *render;
	void *render_private;
};

struct render_interface {
	const char *name;

	/* creates private render data (called after create_win) */
	void (*create_private)(struct panel *p);
	void (*free_private)(struct panel *p);

	/* creates p->cr (widgets are rendered to that context) */
	void (*create_dc)(struct panel *p);

	/* function for blitting everything to the panel (or it's bg) */
	void (*blit)(struct panel *p, int x, int y, unsigned int w, unsigned int h);

	/* background property change */
	void (*update_bg)(struct panel *p);

	/* expose event */
	void (*expose)(struct panel *p);

	/* panel resize */
	void (*panel_resize)(struct panel *p);
};

extern struct render_interface render_normal;
extern struct render_interface render_pseudo;

void init_panel(struct panel *panel, struct config_format_tree *tree,
		int monitor);
void free_panel(struct panel *panel);
void reconfigure_free_panel(struct panel *panel, struct widget_stash *stash);
void reconfigure_panel(struct panel *panel, struct config_format_tree *tree,
		       struct widget_stash *stash, int monitor);
void reconfigure_panel_config(struct panel *panel);
void reconfigure_widgets(struct panel *panel);
void panel_main_loop(struct panel *panel);

void recalculate_widgets_sizes(struct panel *panel);
int check_mbutton_condition(struct panel *panel, int mbutton, unsigned int condition);

/* event dispatchers */
void disp_button_press_release(struct panel *p, XButtonEvent *e);
void disp_motion_notify(struct panel *p, XMotionEvent *e);
void disp_property_notify(struct panel *p, XPropertyEvent *e);
void disp_enter_leave_notify(struct panel *p, XCrossingEvent *e);
void disp_client_msg(struct panel *p, XClientMessageEvent *e);
void disp_win_destroy(struct panel *p, XDestroyWindowEvent *e);
void disp_configure(struct panel *p, XConfigureEvent *e);
