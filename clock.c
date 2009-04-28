#include <time.h>
#include "builtin-widgets.h"

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
		struct config_format_tree *tree);
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
	clock_tick,
	0, /* prop_change */
	0, /* mouse_enter */
	0, /* mouse_leave */
	0, /* mouse_motion */
	0, /* dnd_start */
	0, /* dnd_drag */
	0 /* dnd_drop */
};

void register_clock()
{
	register_widget_interface(&clock_interface);
}

/**************************************************************************
  Clock theme
**************************************************************************/

static int parse_clock_theme(struct clock_theme *ct, 
		struct config_format_entry *e, struct config_format_tree *tree)
{
	if (parse_text_info_named(&ct->font, "font", e, 1))
		return -1;

	parse_triple_image_named(&ct->background, "background", e, tree, 0);
	ct->time_format = parse_string("time_format", e, "%H:%M:%S");

	return 0;
}

static void free_clock_theme(struct clock_theme *ct)
{
	free_triple_image(&ct->background);
	free_text_info(&ct->font);
	xfree(ct->time_format);
}

/**************************************************************************
  Clock interface
**************************************************************************/

static int create_widget_private(struct widget *w, struct config_format_entry *e, 
		struct config_format_tree *tree)
{
	struct clock_widget *cw = xmallocz(sizeof(struct clock_widget));
	if (parse_clock_theme(&cw->theme, e, tree)) {
		xfree(cw);
		XWARNING("Failed to parse clock theme");
		return -1;
	}

	/* get widget width */
	int text_width = 0;
	int pics_width = 0;

	char buftime[128];
	struct tm tm;
	CLEAR_STRUCT(&tm);
	strftime(buftime, sizeof(buftime), cw->theme.time_format, &tm);

	text_extents(w->panel->layout, cw->theme.font.pfd, 
			buftime, &text_width, 0);

	/* background is drawn only if the center is here */
	if (cw->theme.background.center) {
		pics_width += image_width(cw->theme.background.left);
		pics_width += image_width(cw->theme.background.right);
	}
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
	struct clock_widget *cw = (struct clock_widget*)w->private;

	/* time */
	char buftime[128];
	time_t current_time;
	current_time = time(0);
	strftime(buftime, sizeof(buftime), cw->theme.time_format, 
			localtime(&current_time));

	/* drawing */
	cairo_t *cr = w->panel->cr;
	int x = w->x;

	/* calcs */
	int leftw = 0;
	int rightw = 0;
	int centerw = w->width;

	/* draw background only if the center image is here */
	if (cw->theme.background.center) {
		leftw += image_width(cw->theme.background.left);
		rightw += image_width(cw->theme.background.right);
		centerw -= leftw + rightw;

		/* left part */
		if (leftw)
			blit_image(cw->theme.background.left, cr, x, 0);
		x += leftw;

		/* center part */
		pattern_image(cw->theme.background.center, cr, x, 0, 
				centerw);
		x += centerw;

		/* right part */
		if (rightw)
			blit_image(cw->theme.background.right, cr, x, 0);
		x -= centerw;
	}

	/* text */
	draw_text(cr, w->panel->layout, &cw->theme.font, buftime, 
			x, 0, centerw, w->panel->height);
}

static void clock_tick(struct widget *w)
{
	struct clock_widget *cw = (struct clock_widget*)w->private;

	static char buflasttime[128];
	char buftime[128];
	
	time_t current_time;
	current_time = time(0);
	strftime(buftime, sizeof(buftime), cw->theme.time_format, localtime(&current_time));
	if (!strcmp(buflasttime, buftime))
		return;
	strcpy(buflasttime, buftime);

	w->needs_expose = 1;
}

