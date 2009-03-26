#include <string.h>
#include "gui.h"

static int parse_position(const char *pos)
{
	if (strcmp("top", pos) == 0)
		return PANEL_POSITION_TOP;
	else if (strcmp("bottom", pos) == 0)
		return PANEL_POSITION_BOTTOM;
	return PANEL_POSITION_TOP;
}

int panel_theme_load(struct panel_theme *theme, struct theme_format_tree *tree)
{
	memset(theme, 0, sizeof(struct panel_theme));
	struct theme_format_entry *e = theme_format_find_entry(&tree->root, "panel");
	if (!e)
		return 1;

	const char *v;
	v = theme_format_find_entry_value(e, "position");
	if (v)
		theme->position = parse_position(v);

	return 0;
}
