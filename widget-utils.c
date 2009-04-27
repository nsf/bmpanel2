#include <stdio.h>
#include <alloca.h>
#include "widget-utils.h"

/**************************************************************************
  Parsing utils
**************************************************************************/

static int parse_image_dimensions(int *x, int *y, int *w, int *h,
		struct config_format_entry *e)
{
	/* XXX */
	struct config_format_entry *ee = find_config_format_entry(e, "xywh");
	if (ee && ee->value) {
		if (4 == sscanf(ee->value, "%d:%d:%d:%d", x, y, w, h))
			return 0;
		else
			XWARNING("Failed to parse \"xywh\" value, "
				 "the format is: \"%%d:%%d:%%d:%%d\" (line: %u)",
				 ee->line);
	}
	return -1;
}


static int parse_color(unsigned char *out, struct config_format_entry *e)
{
	out[0] = out[1] = out[2] = 0;
	struct config_format_entry *ee = find_config_format_entry(e, "color");
	if (ee && ee->value) {
		if (3 == sscanf(ee->value, "%d %d %d", &out[0], &out[1], &out[2]))
			return 0;
		else
			XWARNING("Failed to parse \"color\" value, "
				 "the format is: \"%%d %%d %%d\" (line: %u)",
				 ee->line);
	}
	return -1;
}

int parse_2ints(int *out, const char *name, struct config_format_entry *e)
{
	out[0] = out[1] = 0;
	struct config_format_entry *ee = find_config_format_entry(e, name);
	if (ee && ee->value) {
		if (2 == sscanf(ee->value, "%d %d", &out[0], &out[1]))
			return 0;
		else
			XWARNING("Failed to parse 2 ints \"%s\" value, "
				 "the format is: \"%%d %%d\" (line: %u)", 
				 name, ee->line);
	}
	return -1;
}

static int parse_align(struct config_format_entry *e)
{
	struct config_format_entry *ee = find_config_format_entry(e, "align");
	if (!ee || !ee->value)
		return TEXT_ALIGN_CENTER;

	if (strcmp("left", ee->value) == 0)
		return TEXT_ALIGN_LEFT;
	else if (strcmp("right", ee->value) == 0)
		return TEXT_ALIGN_RIGHT;
	else if (strcmp("center", ee->value) == 0)
		return TEXT_ALIGN_CENTER;
	XWARNING("Unknown align type: \"%s\", back to default \"center\""
		 " (line: %d)", ee->value, ee->line);
	return TEXT_ALIGN_CENTER;
}

cairo_surface_t *parse_image_part(struct config_format_entry *e,
				  struct config_format_tree *tree,
				  int required)
{
	cairo_surface_t *img;
	int x,y,w,h;
	x = y = w = h = -1;
	char *file;
	size_t filestrlen = strlen(tree->dir) + 1 + strlen(e->value) + 1;

	/* compute path */
	if (filestrlen > MAX_ALLOCA)
		file = xmalloc(filestrlen);
	else
		file = alloca(filestrlen);
	if (!strcmp(tree->dir, ""))
		strcpy(file, e->value);
	else
		sprintf(file, "%s/%s", tree->dir, e->value);

	/* load file */
	if (parse_image_dimensions(&x,&y,&w,&h,e) == 0)
		img = get_image_part(file,x,y,w,h);
	else
		img = get_image(file);
	
	if (!img) {
		if (required)
			XWARNING("Failed to load image \"%s\" which is required "
				 "(line: %u)", file, e->line);
		else
			XWARNING("Failed to load image \"%s\" (line: %u)", 
				 file, e->line);
	}

	/* free path */
	if (filestrlen > MAX_ALLOCA)
		xfree(file);

	return img;
}

cairo_surface_t *parse_image_part_named(const char *name, 
					struct config_format_entry *e,
					struct config_format_tree *tree, 
					int required)
{
	struct config_format_entry *ee = find_config_format_entry(e, name);
	if (!ee) {
		if (required)
			required_entry_not_found(e, name);
		return 0;
	}
	return parse_image_part(ee, tree, required);
}

int parse_triple_image(struct triple_image *tbt, struct config_format_entry *e, 
		       struct config_format_tree *tree)
{
	tbt->center = parse_image_part_named("center", e, tree, 1);
	if (!tbt->center)
		return -1;

	tbt->left = parse_image_part_named("left", e, tree, 0);
	tbt->right = parse_image_part_named("right", e, tree, 0);
	return 0;
}

int parse_triple_image_named(struct triple_image *tri, const char *name, 
			     struct config_format_entry *e, 
			     struct config_format_tree *tree, int required)
{
	struct config_format_entry *ee = find_config_format_entry(e, name);
	if (!ee) {
		if (required)
			required_entry_not_found(e, name);
		return -1;
	}

	return parse_triple_image(tri, ee, tree);
}

void free_triple_image(struct triple_image *tbt)
{
	cairo_surface_destroy(tbt->center);
	if (tbt->left) 
		cairo_surface_destroy(tbt->left);
	if (tbt->right) 
		cairo_surface_destroy(tbt->right);
}

int parse_text_info(struct text_info *out, struct config_format_entry *e)
{
	out->pfd = pango_font_description_from_string(e->value);
	parse_color(out->color, e);
	parse_2ints(out->offset, "offset", e);
	out->align = parse_align(e);

	return 0;
}

int parse_text_info_named(struct text_info *out, const char *name,
			  struct config_format_entry *e, int required)
{
	struct config_format_entry *ee = find_config_format_entry(e, name);
	if (!ee) {
		if (required)
			required_entry_not_found(e, name);
		return -1;
	}

	return parse_text_info(out, ee);
}

void free_text_info(struct text_info *fi)
{
	pango_font_description_free(fi->pfd);
}

int parse_int(const char *name, struct config_format_entry *e, int def)
{
	const char *v = find_config_format_entry_value(e, name);
	int i;
	if (v) {
		if (1 == sscanf(v, "%d", &i))
			return i;
	}
	return def;
}

char *parse_string(const char *name, struct config_format_entry *e, const char *def)
{
	const char *v = find_config_format_entry_value(e, name);
	if (v)
		return xstrdup(v);
	return xstrdup(def);
}

void required_entry_not_found(struct config_format_entry *e, const char *name)
{
	char buf[2048];
	memset(buf, 0, sizeof(buf));
	config_format_entry_path(buf, sizeof(buf), e);
	XWARNING("Failed to find \"%s/%s\" entry which is required",
		 buf, name);
}

/**************************************************************************
  Drawing utils
**************************************************************************/

void blit_image(cairo_surface_t *src, cairo_t *dest, int dstx, int dsty)
{
	size_t sh = cairo_image_surface_get_height(src);
	size_t sw = cairo_image_surface_get_width(src);

	cairo_save(dest);
	cairo_set_source_surface(dest, src, dstx, dsty);
	cairo_rectangle(dest, dstx, dsty, sw, sh);
	cairo_clip(dest);
	cairo_paint(dest);
	cairo_restore(dest);
}

void pattern_image(cairo_surface_t *src, cairo_t *dest, 
		int dstx, int dsty, int w)
{
	size_t sh = cairo_image_surface_get_height(src);

	cairo_save(dest);
	cairo_set_source_surface(dest, src, 0, 0);
	cairo_pattern_set_extend(cairo_get_source(dest), CAIRO_EXTEND_REPEAT);
	cairo_rectangle(dest, dstx, dsty, w, sh);
	cairo_clip(dest);
	cairo_paint(dest);
	cairo_restore(dest);
}

void draw_text(cairo_t *cr, PangoLayout *dest, struct text_info *ti, 
		const char *text, int x, int y, int w, int h)
{
	PangoRectangle r;
	int offsetx = 0, offsety = 0;

	cairo_save(cr);
	cairo_set_source_rgb(cr,
			(double)ti->color[0] / 255.0,
			(double)ti->color[1] / 255.0,
			(double)ti->color[2] / 255.0);
	pango_layout_set_font_description(dest, ti->pfd);
	pango_layout_set_text(dest, text, -1);
	
	pango_layout_get_pixel_extents(dest, 0, &r);

	offsety = (h - r.height) / 2;
	switch (ti->align) {
	default:
	case TEXT_ALIGN_CENTER:
		offsetx = (w - r.width) / 2;
		break;
	case TEXT_ALIGN_LEFT:
		offsetx = 0;
		break;
	case TEXT_ALIGN_RIGHT:
		offsetx = w - r.width;
		break;
	}
	
	offsetx += ti->offset[0];
	offsety += ti->offset[1];

	cairo_translate(cr, x, y);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_translate(cr, offsetx, offsety);
	cairo_clip(cr);
	pango_cairo_update_layout(cr, dest);
	pango_cairo_show_layout(cr, dest);
	cairo_restore(cr);
}

void text_extents(PangoLayout *layout, PangoFontDescription *font,
		const char *text, int *w, int *h)
{
	PangoRectangle r;
	pango_layout_set_font_description(layout, font);
	pango_layout_set_text(layout, text, -1);
	pango_layout_get_pixel_extents(layout, 0, &r);
	if (w)
		*w = r.width;
	if (h)
		*h = r.height;
}

/**************************************************************************
  X imaging utils
**************************************************************************/

static cairo_user_data_key_t surface_data_key;

static void free_custom_surface_data(void *ptr)
{
	xfree(ptr);
}

static cairo_surface_t *get_icon_from_netwm(long *data)
{
	cairo_surface_t *ret = 0;
	uint32_t *array = 0; 
	uint32_t w,h,size,i;
	long *locdata = data;

	w = *locdata++;
	h = *locdata++;
	size = w * h;

	/* convert netwm icon format to cairo data */
	array = xmalloc(sizeof(uint32_t) * size);
	for (i = 0; i < size; ++i) {
		unsigned char *a, *d;
		a = (char*)&array[i];
		d = (char*)&locdata[i];
		a[0] = d[0];
		a[1] = d[1];
		a[2] = d[2];
		a[3] = d[3];
		/* premultiply alpha */
		a[0] *= (float)d[3] / 255.0f;
		a[1] *= (float)d[3] / 255.0f;
		a[2] *= (float)d[3] / 255.0f;
	}

	/* TODO: I'm pretending here, that stride is w*4, but it should
	 * be tested/verified 
	 */
	ret = cairo_image_surface_create_for_data((unsigned char*)array, 
			CAIRO_FORMAT_ARGB32, w, h, w*4);		
	if (cairo_surface_status(ret) != CAIRO_STATUS_SUCCESS)
		goto get_icon_from_netwm_error;
	else {
		cairo_status_t st;
		st = cairo_surface_set_user_data(ret, &surface_data_key, 
				array, free_custom_surface_data);
		if (st != CAIRO_STATUS_SUCCESS)
			goto get_icon_from_netwm_error;
	}

	return ret;

get_icon_from_netwm_error:
	cairo_surface_destroy(ret);
	xfree(array);
	return 0;
}

static cairo_surface_t *get_icon_from_pixmap(struct x_connection *c, 
		Pixmap icon, Pixmap icon_mask)
{
	Window root_ret;
	int x = 0, y = 0;
	unsigned int w = 0, h = 0, d = 0, bw = 0;
	cairo_surface_t *ret = 0;
	cairo_surface_t *sicon, *smask;
	
	XGetGeometry(c->dpy, icon, &root_ret, 
			&x, &y, &w, &h, &bw, &d);

	sicon = cairo_xlib_surface_create(c->dpy, icon, 
			c->default_visual, w, h);
	if (cairo_surface_status(sicon) != CAIRO_STATUS_SUCCESS)
		goto get_icon_from_pixmap_error_sicon;

	smask = cairo_xlib_surface_create_for_bitmap(c->dpy, icon_mask,
			DefaultScreenOfDisplay(c->dpy), w, h);
	if (cairo_surface_status(smask) != CAIRO_STATUS_SUCCESS)
		goto get_icon_from_pixmap_error_smask;

	ret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	if (cairo_surface_status(ret) != CAIRO_STATUS_SUCCESS)
		goto get_icon_from_pixmap_error_ret;

	cairo_t *cr = cairo_create(ret);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
		goto get_icon_from_pixmap_error_cr;

	/* fill with transparent (alpha == 0) */
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_paint(cr);

	/* draw icon with mask */
	cairo_set_source_surface(cr, sicon, 0, 0);
	cairo_mask_surface(cr, smask, 0, 0);

	/* clean up */
	cairo_destroy(cr);
	cairo_surface_destroy(sicon);
	cairo_surface_destroy(smask);

	return ret;

get_icon_from_pixmap_error_cr:
	cairo_surface_destroy(ret);
get_icon_from_pixmap_error_ret:
	cairo_surface_destroy(smask);
get_icon_from_pixmap_error_smask:
	cairo_surface_destroy(sicon);
get_icon_from_pixmap_error_sicon:
	return 0;
}

cairo_surface_t *get_window_icon(struct x_connection *c, Window win,
		cairo_surface_t *default_icon)
{
	cairo_surface_t *ret = 0;
	
	int num = 0;
	long *data = x_get_prop_data(c, win, c->atoms[XATOM_NET_WM_ICON],
			XA_CARDINAL, &num);

	/* TODO: look for best sized icon? */
	if (data) {
		ret = get_icon_from_netwm(data);
		XFree(data);
	}

	if (!ret) {
	        XWMHints *hints = XGetWMHints(c->dpy, win);
		if (hints) {
			if (hints->flags & IconPixmapHint) {
				ret = get_icon_from_pixmap(c, hints->icon_pixmap,
						hints->icon_mask);
			}
	        	XFree(hints);
		}
	}

	if (!ret) {
		cairo_surface_reference(default_icon);
		return default_icon;
	}

	double w = cairo_image_surface_get_width(default_icon);
	double h = cairo_image_surface_get_height(default_icon);

	double ow = cairo_image_surface_get_width(ret);
	double oh = cairo_image_surface_get_height(ret);

	cairo_surface_t *sizedret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			w, h);
	if (cairo_surface_status(sizedret) != CAIRO_STATUS_SUCCESS)
		goto get_window_icon_error_sizedret;

	cairo_t *cr = cairo_create(sizedret);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
		goto get_window_icon_error_cr;

	cairo_scale(cr, w / ow, h / oh);
	cairo_set_source_surface(cr, ret, 0, 0);
	cairo_paint(cr);

	cairo_destroy(cr);
	cairo_surface_destroy(ret);

	return sizedret;

get_window_icon_error_cr:
	cairo_surface_destroy(sizedret);
get_window_icon_error_sizedret:
	cairo_surface_destroy(ret);
	return 0;
}
