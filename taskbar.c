#include "drawing-utils.h"
#include "builtin-widgets.h"

static struct widget *create_widget(struct theme_format_entry *e, 
		struct theme_format_tree *tree);
static void destroy_widget(struct widget *w);
static void draw(struct widget *w, struct panel *p);

static struct widget_interface taskbar_interface = {
	"taskbar",
	WIDGET_SIZE_FILL,
	create_widget,
	destroy_widget,
	draw
};

static int parse_taskbar_theme(struct taskbar_theme *tt, 
		struct theme_format_entry *e, struct theme_format_tree *tree)
{
	struct theme_format_entry *ee = theme_format_find_entry(e, "idle");
	if (!ee)
		return xerror("Can't find 'idle' taskbar button theme entry");

	if (parse_taskbar_button_theme(&tt->idle, ee, tree))
		return xerror("Can't parse 'idle' taskbar button theme");

	return 0;
}

static void free_taskbar_theme(struct taskbar_theme *tt)
{
	if (tt->idle.left.img) release_image(tt->idle.left.img);
	if (tt->idle.right.img) release_image(tt->idle.right.img);
	if (tt->idle.center.img) release_image(tt->idle.center.img);
}

static struct widget *create_widget(struct theme_format_entry *e, 
		struct theme_format_tree *tree)
{
	struct taskbar_widget *w = xmallocz(sizeof(struct taskbar_widget));
	w->widget.interface = &taskbar_interface;
	if (parse_taskbar_theme(&w->theme, e, tree)) {
		xfree(w);
		return 0;
	}

	return (struct widget*)w;
}

static void draw(struct widget *w, struct panel *p)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w;
	int x = w->x;
	int i;
	for (i = 0; i < 4; ++i) {
		blit_image_part(&tw->theme.idle.left, p->cr, x, w->y);
		x += tw->theme.idle.left.width;
		pattern_image_part(&tw->theme.idle.center, p->cr, 
				x, w->y, 100);
		x += 100;
		blit_image_part(&tw->theme.idle.right, p->cr, x, w->y);
		x += tw->theme.idle.right.width;
	}
}

static void destroy_widget(struct widget *w)
{
	struct taskbar_widget *tw = (struct taskbar_widget*)w;
	free_taskbar_theme(&tw->theme);
	xfree(tw);
}

void register_taskbar()
{
	register_widget_interface(&taskbar_interface);
}
