#ifndef BMPANEL2_BUILTIN_WIDGETS_H 
#define BMPANEL2_BUILTIN_WIDGETS_H

#include "gui.h"
#include "widget-utils.h"
#include "array.h"

/**************************************************************************
  Taskbar
**************************************************************************/

struct taskbar_task {
	struct strbuf name;
	cairo_surface_t *icon;
	Window win;
	int desktop;
	int x;
	int w;
	int geom_x; /* for _NET_WM_ICON_GEOMETRY */
	int geom_w;

	/* I'm using only one name source Atom and I'm watching it for
	 * updates. 
	 */
	Atom name_atom;
	Atom name_type_atom;
};

struct taskbar_state {
	struct triple_image background;
	struct text_info font;
	int icon_offset[2];
};

struct taskbar_theme {
	struct taskbar_state idle;
	struct taskbar_state pressed;
	cairo_surface_t *default_icon;
	int task_max_width;

	cairo_surface_t *separator;
};

struct taskbar_widget {
	struct taskbar_theme theme;
	DECLARE_ARRAY(struct taskbar_task, tasks);
	Window active;
	int desktop;

	Window dnd_win;
	Window taken;

	int task_death_threshold;
	Cursor dnd_cur;
};

extern struct widget_interface taskbar_interface;

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

extern struct widget_interface clock_interface;

/**************************************************************************
  Desktop Switcher
**************************************************************************/

struct desktops_state {
	cairo_surface_t *left_corner;
	struct triple_image background;
	cairo_surface_t *right_corner;

	struct text_info font;
};

struct desktops_desktop {
	char *name;
	int x;
	int w;
	int textw;
};

struct desktops_theme {
	struct desktops_state idle;
	struct desktops_state pressed;
	cairo_surface_t *separator;
};

struct desktops_widget {
	struct desktops_theme theme;
	DECLARE_ARRAY(struct desktops_desktop, desktops);
	int active;
};

extern struct widget_interface desktops_interface;

/**************************************************************************
  Decor
**************************************************************************/

struct decor_widget {
	cairo_surface_t *image;
};

extern struct widget_interface decor_interface;

/**************************************************************************
  Empty
**************************************************************************/

extern struct widget_interface empty_interface;

/**************************************************************************
  Systray
**************************************************************************/

struct systray_icon {
	Window icon;
	Window embedder;
	int mapped;
};

struct systray_widget {
	DECLARE_ARRAY(struct systray_icon, icons);
	Atom tray_selection_atom;
	Window selection_owner;

	/* theme */
	int icon_size[2];
	int icon_offset[2];
	int icon_spacing;
};

extern struct widget_interface systray_interface;

/**************************************************************************
  Launch Bar
**************************************************************************/

struct launchbar_item {
	cairo_surface_t *icon;
	char *execstr;
	int x;
	int w;
};

struct launchbar_widget {
	DECLARE_ARRAY(struct launchbar_item, items);
	int icon_size[2];
	int active;
};

extern struct widget_interface launchbar_interface;

#endif /* BMPANEL2_BUILTIN_WIDGETS_H */
