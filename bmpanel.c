#include <stdio.h>
#include "util.h"
#include "xutil.h"
#include "gui.h"
#include "theme-parser.h"
#include "array.h"

void print_widgets(struct panel *p)
{
	size_t i;
	for (i = 0; i < p->widgets_n; ++i) {
		struct widget *w = &p->widgets[i];
		printf("widget: %s (%d) [%d %d %d %d]\n", w->interface->theme_name, 
				w->interface->size_type, w->x, w->y, w->width,
				w->height);
	}
}

void print_array(int *arr, size_t size)
{
	size_t i;
	for (i = 0; i < size; ++i)
		printf("%d ", arr[i]);
	printf("\n");
}

/*
void check_array()
{
	DECLARE_ARRAY(int, arr);
	
	INIT_ARRAY(arr, 10);
	printf("size: %d, capacity: %d\n", arr_n, arr_alloc);
	
	ENSURE_ARRAY_CAPACITY(arr, 15);
	printf("size: %d, capacity: %d\n", arr_n, arr_alloc);

	ARRAY_APPEND(arr, 1);
	ARRAY_APPEND(arr, 2);
	ARRAY_APPEND(arr, 3);
	ARRAY_APPEND(arr, 4);
	ARRAY_APPEND(arr, 5);
	ARRAY_APPEND(arr, 6);
	ARRAY_APPEND(arr, 7);
	printf("size: %d, capacity: %d\n", arr_n, arr_alloc);
	print_array(arr, arr_n);

	ARRAY_INSERT_AFTER(arr, 3, 31337);
	printf("size: %d, capacity: %d\n", arr_n, arr_alloc);
	print_array(arr, arr_n);
	
	ARRAY_PREPEND(arr, 666);
	printf("size: %d, capacity: %d\n", arr_n, arr_alloc);
	print_array(arr, arr_n);
	
	SHRINK_ARRAY(arr);
	printf("size: %d, capacity: %d\n", arr_n, arr_alloc);
	print_array(arr, arr_n);

	ARRAY_REMOVE(arr, 2);
	printf("size: %d, capacity: %d\n", arr_n, arr_alloc);
	print_array(arr, arr_n);

	FREE_ARRAY(arr);
}
*/

int main(int argc, char **argv)
{
	struct theme_format_tree tree;
	struct panel p;

	if (0 != load_theme_format_tree(&tree, "."))
		xdie("Failed to load theme file");

	printf("theme from dir: '%s' loaded\n", tree.dir);
	
	register_taskbar();
	register_clock();

	if (0 != create_panel(&p, &tree))
		xdie("Failed to create panel");

	panel_main_loop(&p);

	destroy_panel(&p);
	free_theme_format_tree(&tree);
	clean_image_cache();

	/* check_array(); */

	xmemstat(0, 0, false);
	return EXIT_SUCCESS;
}
