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
	void (*mouse_enter)(struct widget *w);
	void (*mouse_leave)(struct widget *w);
	void (*mouse_motion)(struct widget *w, XMotionEvent *e);
	/* When D'n'D starts, it contains following valid variables:
	 * "taken_on" - widget on which drag was started, this widget recieves
	 *              "dnd_start" message.
	 * "taken_x" 
	 * "taken_y" - where drag was started, coordinates of the mouse
	 *             press event.
	 * "cur_x" 
	 * "cur_y" - where mouse with virtual drag object is now
	 */
	void (*dnd_start)(struct drag_info *di);

	/* D'n'D drag message is sent to the "taken_on" widget. Has the same
	 * valid variables as above. 
	 */
	void (*dnd_drag)(struct drag_info *di);

	/* This message is sent to a "dropped_on" widget first, and then to a
	 * "taken_on" widget.
	 */
	void (*dnd_drop)(struct drag_info *di);
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

int create_panel(struct panel *panel, struct theme_format_tree *tree);
void destroy_panel(struct panel *panel);
void panel_main_loop(struct panel *panel);

void recalculate_widgets_sizes(struct panel *panel);

/* event dispatchers */
void disp_button_press_release(struct panel *p, XButtonEvent *e);
void disp_motion_notify(struct panel *p, XMotionEvent *e);
void disp_property_notify(struct panel *p, XPropertyEvent *e);
void disp_enter_leave_notify(struct panel *p, XCrossingEvent *e);

#endif /* BMPANEL2_GUI_H */
