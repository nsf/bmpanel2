#include "gui.h"

static struct widget *create_widget(struct theme_format_entry *e, 
		struct theme_format_tree *tree)
{
}

static void destroy_widget(struct widget *w)
{
}

static struct widget_class taskbar_interface = {
	"taskbar",
	WIDGET_SIZE_FILL,
	create_widget,
	destroy_widget
};

void register_taskbar()
{
	register_widget_interface(&taskbar_interface);
}
