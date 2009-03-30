#ifndef BMPANEL2_BUILTIN_WIDGETS_H 
#define BMPANEL2_BUILTIN_WIDGETS_H

#include "gui.h"

struct taskbar_task {
	const char *name;
	cairo_surface_t *icon;
	Window win;
};

struct taskbar_button_theme {
	struct image_part left;
	struct image_part center;
	struct image_part right;
};

struct taskbar_theme {
	struct taskbar_button_theme idle;
	struct taskbar_button_theme pressed;
};

struct taskbar_widget {
	struct widget widget;

	size_t tasks_n;
	struct taskbar_task *tasks;

	struct taskbar_theme theme;
};

#endif /* BMPANEL2_BUILTIN_WIDGETS_H */
