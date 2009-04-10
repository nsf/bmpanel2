#ifndef BMPANEL2_BUILTIN_WIDGETS_H 
#define BMPANEL2_BUILTIN_WIDGETS_H

#include "gui.h"
#include "parsing-utils.h"

/**************************************************************************
  Taskbar
**************************************************************************/

struct taskbar_task {
	const char *name;
	cairo_surface_t *icon;
	Window win;
	int desktop;
};

struct taskbar_theme {
	struct triple_image idle;
	struct triple_image pressed;
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
	PangoFontDescription *font;
	unsigned char font_color[3];
};

struct clock_widget {
	struct clock_theme theme;
};

#endif /* BMPANEL2_BUILTIN_WIDGETS_H */
