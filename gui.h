#ifndef BMPANEL2_GUI_H 
#define BMPANEL2_GUI_H

#include <cairo.h>
#include "util.h"
#include "theme-parser.h"

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

struct widget_class {
	const char *theme_name;
	int size_type;
};

struct widget {
	struct widget_class *wclass;

	/* rectangle for event dispatching */
	int x;
	int y;
	int width;
	int height;
};

/**************************************************************************
  Panel
**************************************************************************/

#define PANEL_POSITION_TOP 1
#define PANEL_POSITION_BOTTOM 2

struct panel_theme {
	int position;
	cairo_surface_t *background;
	cairo_surface_t *separator;
};

#define PANEL_MAX_WIDGETS 20

struct panel {
	size_t widgets_n;
	struct widget *widgets[PANEL_MAX_WIDGETS];

	struct panel_theme *theme;
};

int panel_theme_load(struct panel_theme *theme, struct theme_format_tree *tree);
void panel_theme_free(struct panel_theme *theme);

int panel_create(struct panel *panel, struct panel_theme *theme);
void panel_destroy(struct panel *panel);

#endif /* BMPANEL2_GUI_H */
