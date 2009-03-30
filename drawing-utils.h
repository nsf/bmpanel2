#ifndef BMPANEL2_DRAWING_UTILS_H 
#define BMPANEL2_DRAWING_UTILS_H

#include "gui.h"

void blit_image_part(struct image_part *src, cairo_t *dest, int dstx, int dsty);
void pattern_image_part(struct image_part *src, cairo_t *dest, 
		int dstx, int dsty, int w);

#endif /* BMPANEL2_DRAWING_UTILS_H */
