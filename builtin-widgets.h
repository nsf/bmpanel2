#ifndef BMPANEL2_BUILTIN_WIDGETS_H 
#define BMPANEL2_BUILTIN_WIDGETS_H

#include "gui.h"

/**************************************************************************
  Taskbar
**************************************************************************/

struct taskbar_task {
	const char *name;
	cairo_surface_t *icon;
	Window win;
};

struct taskbar_button_theme {
	cairo_surface_t *left;
	cairo_surface_t *center;
	cairo_surface_t *right;
};

struct taskbar_theme {
	struct taskbar_button_theme idle;
	struct taskbar_button_theme pressed;
};

struct taskbar_widget {
	struct taskbar_theme theme;
	size_t tasks_n;
	struct taskbar_task *tasks;
};

#endif /* BMPANEL2_BUILTIN_WIDGETS_H */
