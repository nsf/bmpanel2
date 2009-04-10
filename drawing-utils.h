#ifndef BMPANEL2_DRAWING_UTILS_H 
#define BMPANEL2_DRAWING_UTILS_H

#include "gui.h"

void blit_image(cairo_surface_t *src, cairo_t *dest, int dstx, int dsty);
void pattern_image(cairo_surface_t *src, cairo_t *dest, 
		int dstx, int dsty, int w);

#define TEXT_ALIGN_CENTER 0
#define TEXT_ALIGN_LEFT 1
#define TEXT_ALIGN_RIGHT 2

void draw_text(cairo_t *cr, PangoLayout *dest, PangoFontDescription *font, 
		const char *text, unsigned char *color, int x, int y, 
		int w, int h, int align);
void text_extents(PangoLayout *layout, PangoFontDescription *font,
		const char *text, int *w, int *h);

#endif /* BMPANEL2_DRAWING_UTILS_H */
