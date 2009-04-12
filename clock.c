#include <time.h>
#include <string.h>
#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree);
static void destroy_widget_private(struct widget *w);
static void draw(struct widget *w);
static void clock_tick(struct widget *w);

static struct widget_interface clock_interface = {
	"clock",
	WIDGET_SIZE_CONSTANT,
	create_widget_private,
	destroy_widget_private,
	draw,
	0, /* button_click */
	clock_tick, /* clock_tick */
	0, /* prop_change */
};

void register_clock()
{
	register_widget_interface(&clock_interface);
}

/**************************************************************************
  Clock theme
**************************************************************************/

static int parse_clock_theme(struct clock_theme *ct, 
		struct theme_format_entry *e, struct theme_format_tree *tree)
{
	if (parse_triple_image(&ct->background, "background", e, tree))
		return xerror("Can't parse 'background' clock triple");

	if (parse_font_info(&ct->font, "font", e, tree)) {
		free_triple_image(&ct->background);
		return xerror("Can't parse 'font' clock entry");
	}

	return 0;
}

static void free_clock_theme(struct clock_theme *ct)
{
	free_triple_image(&ct->background);
	free_font_info(&ct->font);
}

/**************************************************************************
  Clock interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct theme_format_entry *e, 
		struct theme_format_tree *tree)
{
	struct clock_widget *cw = xmallocz(sizeof(struct clock_widget));
	if (parse_clock_theme(&cw->theme, e, tree)) {
		xfree(cw);
		return -1;
	}

	/* get widget width */
	int text_width;
	int pics_width;
	text_extents(w->panel->layout, cw->theme.font.pfd, "00:00:00", &text_width, 0);
	pics_width = cairo_image_surface_get_width(cw->theme.background.left) +
		cairo_image_surface_get_width(cw->theme.background.right);
	w->width = text_width + pics_width;
	w->private = cw;
	return 0;
}

static void destroy_widget_private(struct widget *w)
{
	struct clock_widget *cw = (struct clock_widget*)w->private;
	free_clock_theme(&cw->theme);
	xfree(cw);
}

static void draw(struct widget *w)
{
	char buftime[128];
	time_t current_time;

	struct clock_widget *cw = (struct clock_widget*)w->private;
	cairo_t *cr = w->panel->cr;
	int x = w->x;

	int leftw = cairo_image_surface_get_width(cw->theme.background.left);
	int rightw = cairo_image_surface_get_width(cw->theme.background.right);
	int centerw = w->width - leftw - rightw;

	blit_image(cw->theme.background.left, cr, x, w->y);
	x += leftw;
	pattern_image(cw->theme.background.center, cr, x, w->y, 
			centerw);
	x += centerw;
	blit_image(cw->theme.background.right, cr, x, w->y);
	
	x = w->x + leftw;
	
	current_time = time(0);
	strftime(buftime, sizeof(buftime), "%H:%M:%S", localtime(&current_time));

	draw_text(cr, w->panel->layout, cw->theme.font.pfd, buftime, 
			cw->theme.font.color, x, w->y, 
			centerw, w->height, TEXT_ALIGN_CENTER);
}

static void clock_tick(struct widget *w)
{
	static char buflasttime[128];
	char buftime[128];
	time_t current_time;
	current_time = time(0);
	strftime(buftime, sizeof(buftime), "%H:%M:%S", localtime(&current_time));
	if (!strcmp(buflasttime, buftime))
		return;
	strcpy(buflasttime, buftime);

	w->needs_expose = 1;
}
