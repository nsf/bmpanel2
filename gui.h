#ifndef BMPANEL2_GUI_H 
#define BMPANEL2_GUI_H

#include <cairo.h>
#include "util.h"
#include "xutil.h"
#include "theme-parser.h"

/**************************************************************************
  Image cache
**************************************************************************/

struct image {
	char *filename;
	cairo_surface_t	*surface;
	int ref_count;
};

#define IMAGE_INC(image_ptr) ((image_ptr)->ref_count++)
#define IMAGE_DEC(image_ptr) 			\
do {						\
	(image_ptr)->ref_count--;		\
	if ((image_ptr)->ref_count == 0)	\
		free_image(image_ptr);		\
} while (0)					

struct image *get_image(const char *path);
void free_image(struct image *img); /* don't use directly, use IMAGE_DEC */
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

struct widget_interface {
	const char *theme_name;
	int size_type;
	
	struct widget *(*create_widget)(struct theme_format_entry *e, 
			struct theme_format_tree *tree);
	void (*destroy_widget)(struct widget *w);
};

struct widget {
	struct widget_interface *interface;

	/* rectangle for event dispatching */
	int x;
	int y;
	int width;
	int height;
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

#define PANEL_POSITION_TOP 1
#define PANEL_POSITION_BOTTOM 2

struct panel_theme {
	int position;
	cairo_surface_t *background;
	cairo_surface_t *separator;
	int height;
};

#define PANEL_MAX_WIDGETS 20

struct panel {
	Window win;

	size_t widgets_n;
	struct widget *widgets[PANEL_MAX_WIDGETS];

	struct panel_theme theme;
	struct x_connection connection;
};

int panel_create(struct panel *panel, struct theme_format_tree *tree);
void panel_destroy(struct panel *panel);

#endif /* BMPANEL2_GUI_H */
