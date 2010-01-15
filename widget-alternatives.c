#include "gui.h"
#include "widget-utils.h"

/**************************************************************************
  Alternative widgets
**************************************************************************/

struct widget_alternative_def {
	int user_has;
	int theme_has;
	int used;
	const char *name;
};

#define WIDGET_ALTERNATIVE_DEF(name) \
	{0, 0, 0, name}

#define WIDGET_ALTERNATIVE_DEF_END \
	{0, 0, 0, 0}

/* for example, not used */
static struct widget_alternative_def desktops_alternatives[] = {
	WIDGET_ALTERNATIVE_DEF("desktop_switcher"),
	WIDGET_ALTERNATIVE_DEF("pager"),
	WIDGET_ALTERNATIVE_DEF_END
};

static struct widget_alternative_def *widget_alternatives[] = {
	desktops_alternatives,
	0
};

/**************************************************************************
  Alternative widgets functions
**************************************************************************/

static int group_has_user_preference(struct widget_alternative_def *group)
{
	while (group->name) {
		if (group->user_has)
			return 1;
		group++;
	}
	return 0;
}

static struct widget_alternative_def *
group_has_user_theme_match(struct widget_alternative_def *group)
{
	while (group->name) {
		if (group->user_has && group->theme_has)
			return group;
		group++;
	}
	return 0;
}

static struct widget_alternative_def *
find_used_widget(struct widget_alternative_def *group)
{
	while (group->name) {
		if (group->used)
			return group;
		group++;
	}
	return 0;
}

static struct widget_alternative_def *
find_widget_alternative_def(const char *name, struct widget_alternative_def **group_out)
{
	struct widget_alternative_def **wa = widget_alternatives;
	while (*wa) {
		struct widget_alternative_def *group = (*wa);
		struct widget_alternative_def *wad = group;
		while (wad->name) {
			if (strcmp(wad->name, name) == 0) {
				if (group_out)
					*group_out = group;
				return wad;
			}
			wad++;
		}
		wa++;
	}
	return 0;
}

static void match_user_preference(const char *pref) 
{
	struct widget_alternative_def *group;
	struct widget_alternative_def *wad;

	wad = find_widget_alternative_def(pref, &group);
	if (!wad)
		return;

	if (group_has_user_preference(group)) {
		printf("warning: group already has user preference, ignoring '%s'\n", pref);
		return;
	}

	wad->user_has = 1;
}

static void match_theme_has(const char *theme_name) 
{
	struct widget_alternative_def *wad;

	wad = find_widget_alternative_def(theme_name, 0);
	if (!wad)
		return;

	wad->theme_has = 1;
}

static void match_theme_has_widget_alternatives(struct config_format_tree *tree)
{
	size_t i;
	for (i = 0; i < tree->root.children_n; ++i) {
		struct config_format_entry *e = &tree->root.children[i];
		struct widget_interface *we = lookup_widget_interface(e->name);
		if (!we) 
			continue;

		match_theme_has(e->name);
	}
}

void update_alternatives_preference(char *prefstr, struct config_format_tree *tree)
{
	for_each_word(prefstr, match_user_preference);
	match_theme_has_widget_alternatives(tree);
}

int validate_widget_for_alternatives(const char *theme_name)
{
	struct widget_alternative_def *group;
	struct widget_alternative_def *wad;
	struct widget_alternative_def *match;

	wad = find_widget_alternative_def(theme_name, &group);
	if (!wad) /* if we can't find widget/group with that theme_name, then it's ok */
		return 1;

	match = group_has_user_theme_match(group);
	if (match) {
		if (match == wad)
			return 1;
		else
			return 0;
	} else {
		match = find_used_widget(group);
		if (!match) {
			wad->used = 1;
			return 1;
		}
	}
	return 0;
}

void reset_alternatives()
{
	struct widget_alternative_def **wa = widget_alternatives;
	while (*wa) {
		struct widget_alternative_def *wad = (*wa);
		while (wad->name) {
			wad->user_has = wad->theme_has = wad->used = 0;
			wad++;
		}
		wa++;
	}
}

#if 0
static void print_check_widget(const char *wname)
{
	int ok = check_widget(wname);
	printf("!%s!: %d\n", wname, ok);
}

int main(int argc, char **argv)
{
	print_alternatives();
	char preferred_alts[] = "pager taskbar2";
	match_theme_has("panel");
	match_theme_has("desktop_switcher");
	match_theme_has("pager");
	match_theme_has("taskbar");
	match_user_preferred_widget_alternatives(preferred_alts);

	print_check_widget("panel");
	print_check_widget("desktop_switcher");
	print_check_widget("pager");
	print_check_widget("taskbar");
	print_check_widget("systray");
	print_check_widget("clock");

	print_alternatives();
	return 0;
}
#endif
