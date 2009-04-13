#ifndef BMPANEL2_BUILTIN_WIDGETS_H 
#define BMPANEL2_BUILTIN_WIDGETS_H

#include "gui.h"
#include "widget-utils.h"

/**************************************************************************
  Taskbar
**************************************************************************/

struct taskbar_task {
	const char *name;
	cairo_surface_t *icon;
	Window win;
	int desktop;
};

struct taskbar_state {
	struct triple_image background;
	struct text_info font;
};

struct taskbar_theme {
	struct taskbar_state idle;
	struct taskbar_state pressed;
};

struct taskbar_widget {
	struct taskbar_theme theme;
	size_t tasks_n;
	struct taskbar_task *tasks;
	int pressed;
};

/**************************************************************************
  Clock
**************************************************************************/

struct clock_theme {
	struct triple_image background;
	struct text_info font;
	int text_spacing;
	int spacing;
	char *time_format;
};

struct clock_widget {
	struct clock_theme theme;
};

#endif /* BMPANEL2_BUILTIN_WIDGETS_H */
