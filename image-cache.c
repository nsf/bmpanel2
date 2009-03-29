#include "gui.h"

#define IMAGES_CACHE_SIZE 128

static size_t images_cache_n;
static struct image *images_cache[IMAGES_CACHE_SIZE];

static struct image *load_image_from_file(const char *path)
{
	cairo_surface_t *surface = cairo_image_surface_create_from_png(path);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		xwarning("Failed to load cairo png surface: %s", path);
		return 0;
	}

	struct image *img = xmalloc(sizeof(struct image));
	img->filename = xstrdup(path);
	img->surface = surface;
	img->ref_count = 0;
	return img;
}

static struct image *find_image_in_cache(const char *path)
{
	size_t i;
	for (i = 0; i < images_cache_n; ++i) {
		if (strcmp(images_cache[i]->filename, path) == 0)
			return images_cache[i];
	}
	return 0;
}

static void try_add_image_to_cache(struct image *img)
{
	if (images_cache_n == IMAGES_CACHE_SIZE)
		return;

	acquire_image(img);
	images_cache[images_cache_n++] = img;	
}

void free_image(struct image *img)
{
	xfree(img->filename);
	cairo_surface_destroy(img->surface);
	xfree(img);
}

struct image *get_image(const char *path)
{
	struct image *img = find_image_in_cache(path);	
	if (img) 
		return img;

	img = load_image_from_file(path);
	if (img) {
		try_add_image_to_cache(img);
		return img;
	}
	return 0;
}

void clean_image_cache()
{
	size_t i = 0;
	while (i < images_cache_n) {
		if (images_cache[i]->ref_count == 1) {
			release_image(images_cache[i]);
			images_cache[i] = images_cache[images_cache_n-1];
			images_cache_n--;
			continue;
		}
		i++;
	}
}
