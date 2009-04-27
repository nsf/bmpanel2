#ifndef BMPANEL2_BUILTIN_WIDGETS_H 
#define BMPANEL2_BUILTIN_WIDGETS_H

#include "gui.h"
#include "widget-utils.h"
#include "array.h"

/**************************************************************************
  Taskbar
**************************************************************************/

struct taskbar_task {
	char *name;
	cairo_surface_t *icon;
	Window win;
	int desktop;
	int x;
	int w;
};

struct taskbar_state {
	struct triple_image background;
	struct text_info font;
};

struct taskbar_theme {
	struct taskbar_state idle;
	struct taskbar_state pressed;
	cairo_surface_t *default_icon;
	int icon_offset[2];

	cairo_surface_t *separator;
};

struct taskbar_widget {
	struct taskbar_theme theme;
	DECLARE_ARRAY(struct taskbar_task, tasks);
	Window active;
	int desktop;

	Window dnd_win;
	Window taken;
};

/**************************************************************************
  Clock
**************************************************************************/

struct clock_theme {
	struct triple_image background;
	struct text_info font;
	char *time_format;
};

struct clock_widget {
	struct clock_theme theme;
};

/**************************************************************************
  Desktop switcher
**************************************************************************/

struct desktops_state {
	cairo_surface_t *left_corner;
	struct triple_image background;
	cairo_surface_t *right_corner;
	struct text_info font;
};

struct desktops_theme {
	struct desktops_state idle;
	struct desktops_state pressed;
};

/**************************************************************************
  Decor
**************************************************************************/

struct decor_widget {
	cairo_surface_t *image;
};

#endif /* BMPANEL2_BUILTIN_WIDGETS_H */
