#include "gui.h"
#include "builtin-widgets.h"

static struct widget_interface *widget_interfaces[] = {
	&desktops_interface,
	&taskbar_interface,
	&clock_interface,
	&decor_interface,
	&systray_interface,
	&launchbar_interface,
	&empty_interface,
	0
};

struct widget_interface *lookup_widget_interface(const char *themename)
{
	struct widget_interface **wi = widget_interfaces;
	while (*wi) {
		if (strcmp(themename, (*wi)->theme_name) == 0)
			return (*wi);
		wi++;
	}
	return 0;
}
