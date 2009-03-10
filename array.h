#ifndef BMPANEL2_ARRAY_H 
#define BMPANEL2_ARRAY_H

#include <string.h>
#include "util.h"

/*
 * Dynamic array is a collection of three variables:
 * 	type *name; 		- pointer to a memory blob (the array data)
 * 	size_t name_n;		- number of elements in the array
 *	size_t name_alloc;	- capacity of the array
 *
 * Main states:
 *  empty array (no memory) - 
 *  	name = 0
 *  	name_n = 0
 *  	name_alloc = 0
 *
 *  allocated - 
 *  	name = ptr
 *  	name_n = 0
 *  	name_alloc = N
 *
 *  filled -
 *  	name = ptr
 *  	name_n = M
 *  	name_alloc = N
 */

#define DYN_DATA(array) array
#define DYN_SIZE(array) array##_n
#define DYN_CAPACITY(array) array##_alloc

/* Declare dynamic array in a structure or in a global space. */
#define DYN_ARRAY(type, name) 						\
	type *name;							\
	size_t name##_n;						\
	size_t name##_alloc

/* Nullifiy dynamic array (assign all variables to zero). Some macros may cause
 * unexpected behaviour if an array contains garbage. This macro turns a state
 * of an array to a valid state.
 */
#define DYN_ARRAY_NULLIFY(array)					\
do {									\
	array = 0;							\
	array##_n = 0;							\
	array##_alloc = 0;						\
} while (0)

/* Allocate memory enough to hold "size" elements in "array" by using
 * "memory_source". The function should be used only on an array which
 * contains no memory (e.g. uninitialized or nullified). 
 */
#define DYN_ARRAY_ALLOCATE(array, size, memory_source)			\
do {									\
	/* I'm using array##_alloc here to prevent double expr eval */  \
	array##_alloc = size;						\
	array##_n = 0;							\
	array = xmalloc(array##_alloc * sizeof(array[0]),		\
			memory_source);					\
} while (0)

/* Ensure that array can hold "size" elements. For appending, the usage pattern
 * is: "array_n" + N, where N is a number of elements you want to append. Or
 * you can use "DYN_ARRAY_ENSURE_ADD_CAPACITY".
 */
#define DYN_ARRAY_ENSURE_CAPACITY(array, size, memory_source)		\
do {									\
	size_t varsize = size;						\
	if (array##_alloc < varsize) {					\
		size_t newsize = (array##_alloc < 16384)		\
			? array##_alloc * 2				\
			: array##_alloc + 16;				\
		if (newsize < varsize)					\
			newsize = varsize;				\
		void *newmem = xmalloc(newsize * sizeof(array[0]),	\
				memory_source);				\
		if (array##_n) {					\
			memcpy(newmem, array,				\
					array##_n * sizeof(array[0]));	\
		}							\
		if (array)						\
			xfree(array, memory_source);			\
		array = newmem;						\
		array##_alloc = newsize;				\
	}								\
} while (0)

/* The same as above, but ensures additional capacity. */
#define DYN_ARRAY_ENSURE_ADD_CAPACITY(array, add, memory_source)	\
	DYN_ARRAY_ENSURE_CAPACITY(array, array##_n + add, memory_source)

/* If there is more memory than it is required for elements in the
 * array, shrink it. Of course if the array has no elements macro
 * frees the memory.
 */
#define DYN_ARRAY_SHRINK(array, memory_source)				\
do {									\
	if (!array)							\
		;							\
	else if (array##_n == 0) {					\
		xfree(array, memory_source);				\
		array = 0;						\
		array##_alloc = 0;					\
	} else if (array##_n < array##_alloc) {				\
		void *newmem = xmalloc(array##_n * sizeof(array[0]),	\
				memory_source);				\
		memcpy(newmem, array, array##_n * sizeof(array[0]));	\
		xfree(array, memory_source);				\
		array = newmem;						\
		array##_alloc = array##_n;				\
	}								\
} while (0)

/* Free "array" memory using "memory_source". No checks! */
#define DYN_ARRAY_FREE(array, memory_source)				\
do {									\
	xfree(array, memory_source);					\
	array = 0;							\
	array##_n = 0;							\
	array##_alloc = 0;						\
} while (0)

/* Increments the "array" number of elements counter and returns the last
 * element.
 */
#define DYN_ARRAY_NEXT(array)						\
	array[array##_n++]

/* Append an element "what" to the "array". Does all memory routines. */
#define DYN_ARRAY_APPEND(array, what, memory_source)			\
do {									\
	DYN_ARRAY_ENSURE_ADD_CAPACITY(array, 1, memory_source);		\
	DYN_ARRAY_NEXT(array) = what;					\
} while (0)	

#endif /* BMPANEL2_ARRAY_H */
