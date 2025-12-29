#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include "cvector.h"

#include "source/32bit/phtree32_3d.h"
#include "source/32bit/phtree32_6d.h"

/*
 * n-dimensional boxes can be stored as 2*n-dimensional points
 *
 * six dimensional points can represent 3 dimensional axis aligned boxes
 * a 3d box stored as
 * 	box.min = {x, y, z}
 * 	box.max = {x, y, z}
 * in 6 dimensions is
 * 	{box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z}
 */
typedef struct
{
	float x;
	float y;
	float z;
} vector3_t;

typedef struct
{
	float x;
	float y;
	float z;
	float u;
	float v;
	float w;
} vector6_t;

typedef struct
{
	vector3_t position;
	char id;
} point3d_t;

typedef struct
{
	vector6_t position;
	int id;
} point6d_t;

typedef struct
{
	vector3_t min;
	vector3_t max;
	char id;
} box3d_t;

typedef struct
{
	char id;
} element_simple_t;

void convert_to_point_3d (ph3_t* tree, ph3_point_t* out, void* input)
{
	vector3_t* vector = (vector3_t*) input;

	ph3_point_set (tree, out, &vector->x, &vector->y, &vector->z);
}

void convert_to_point_6d (ph6_t* tree, ph6_point_t* out, void* index)
{
	box3d_t* box = index;

	ph6_point_set (tree, out, &box->min.x, &box->min.y, &box->min.z, &box->max.x, &box->max.y, &box->max.z);
}

void convert_to_point_box_6d (ph6_t* tree, ph6_point_t* out, void* index)
{
	vector3_t* vector = index;

	ph6_point_box_set (tree, out, &vector->x, &vector->y, &vector->z);
}

void* element_simple_create (void* unused)
{
	element_simple_t* new_element = calloc (1, sizeof (*new_element));

	if (!new_element)
	{
		return NULL;
	}

	new_element->id = 'z';

	return new_element;
}

void element_simple_destroy (void* element)
{
	free (element);
}

void element_simple_print (void* element_in, void* unused)
{
	element_simple_t* element = element_in;

	printf ("%c, ", element->id);
}

void query_cache_element (void* element_in, void* data)
{
	element_simple_t* element = element_in;
	cvector (element_simple_t*)* list = data;

	cvector_push_back (*list, element);
}

void insert_point_3d (ph3_t* tree, point3d_t* point)
{
	element_simple_t* element = ph3_insert (tree, &point->position);
	element->id = point->id;

	printf ("insert point %c (%.1f, %.1f, %.1f)\n", point->id, point->position.x, point->position.y, point->position.z);
}

void insert_box_3d (ph6_t* tree, box3d_t* box)
{
	element_simple_t* element = ph6_insert (tree, box);
	element->id = box->id;
}

// phtree_float_to_key expects input to be a pointer to a float
phtree_key_t phtree_float_to_key (void* input)
{
	phtree_key_t bits;

	memcpy (&bits, input, sizeof (phtree_key_t));

	// if the float is negative
	// 	convert to two's complement (~bits + 1)
	// 	then & with (PHTREE32_KEY_MAX >> 1)
	// 		which will convert -0 to 0
	if (bits & PHTREE32_SIGN_BIT)
	{
		bits = ((~bits) + 1) & (PHTREE32_KEY_MAX >> 1);
	}
	else
	{
		// if the float is positive
		// 	all we need to do is flip the sign bit to 1
		bits |= PHTREE32_SIGN_BIT;
	}

	return bits;
}

int main ()
{
	ph3_t* tree3d = calloc (1, sizeof (*tree3d));
	ph3_initialize (
		tree3d,
		element_simple_create,
		element_simple_destroy,
		phtree_float_to_key,
		convert_to_point_3d);

	box3d_t box_a = {{-10.0f, -10.0f, -10.0f}, {10.0f, 10.0f, 10.0f}, 'a'};
	box3d_t box_b = {{0.0f, 0.0f, 0.0f}, {20.0f, 20.0f, 20.0f}, 'b'};
	// box_c contains box_b and intersects box_a
	box3d_t box_c = {{-5.0f, -5.0f, -5.0f}, {25.0f, 25.0f, 25.0f}, 'c'};

	// point_a is inside of all boxes
	point3d_t point_a = {{5.0f, 5.0f, 5.0f}, 'a'};
	// point_b is inside of box_b and box_c
	point3d_t point_b = {{15.0f, 15.0f, 15.0f}, 'b'};
	// point_c is not inside of any boxes
	point3d_t point_c = {{-50.0f, -50.0f, -50.0f}, 'c'};

	printf ("*** 3d tree ***\n\n");
	insert_point_3d (tree3d, &point_a);
	insert_point_3d (tree3d, &point_b);
	insert_point_3d (tree3d, &point_c);
	printf ("\n");

	printf ("points in 3d tree:\n  ");
	ph3_for_each (tree3d, element_simple_print, NULL);
	printf ("\n\n");

	ph3_query_t query3d;
	ph3_query_set (tree3d, &query3d, &box_a.min, &box_a.max, query_cache_element);

	cvector (element_simple_t*) query_cache = NULL;
	cvector_init (query_cache, 2, NULL);

	printf ("points in range (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)\n", box_a.min.x, box_a.min.y, box_a.min.z, box_a.max.x, box_a.max.y, box_a.max.z);
	ph3_query (tree3d, &query3d, &query_cache);
	for (int iter = 0; iter < cvector_size (query_cache); iter++)
	{
		printf ("  point %c\n", query_cache[iter]->id);
	}
	printf ("\n");

	cvector_clear (query_cache);

	printf ("points in range (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)\n", box_b.min.x, box_b.min.y, box_b.min.z, box_b.max.x, box_b.max.y, box_b.max.z);
	ph3_query_set (tree3d, &query3d, &box_b.min, &box_b.max, query_cache_element);
	ph3_query (tree3d, &query3d, &query_cache);
	for (int iter = 0; iter < cvector_size (query_cache); iter++)
	{
		printf ("  point %c\n", query_cache[iter]->id);
	}

	cvector_clear (query_cache);
	printf ("\n\n");

	ph3_clear (tree3d);
	free (tree3d);

	printf ("*** 6d tree ***\n\n");
	ph6_t* tree6d = calloc (1, sizeof (*tree6d));
	ph6_initialize (
		tree6d,
		element_simple_create,
		element_simple_destroy,
		phtree_float_to_key,
		convert_to_point_6d,
		convert_to_point_box_6d);

	insert_box_3d (tree6d, &box_a);
	insert_box_3d (tree6d, &box_b);

	printf ("boxes in 6d tree:\n  ");
	ph6_for_each (tree6d, element_simple_print, NULL);
	printf ("\n\n");

	ph6_query_t query6d;

	ph6_query_box_set (tree6d, &query6d, false, &box_c.min, &box_c.max, query_cache_element);
	ph6_query (tree6d, &query6d, &query_cache);

	printf ("boxes contained in box {(%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f)}\n", box_c.min.x, box_c.min.y, box_c.min.z, box_c.max.x, box_c.max.y, box_c.max.z);
	for (int iter = 0; iter < cvector_size (query_cache); iter++)
	{
		printf ("  box %c\n", query_cache[iter]->id);
	}
	printf ("\n");

	cvector_clear (query_cache);

	ph6_query_box_set (tree6d, &query6d, true, &box_c.min, &box_c.max, query_cache_element);
	ph6_query (tree6d, &query6d, &query_cache);

	printf ("boxes intersecting box {(%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f)}\n",  box_c.min.x, box_c.min.y, box_c.min.z, box_c.max.x, box_c.max.y, box_c.max.z);
	for (int iter = 0; iter < cvector_size (query_cache); iter++)
	{
		printf ("  box %c\n", query_cache[iter]->id);
	}
	printf ("\n");

	cvector_clear (query_cache);

	ph6_query_box_point_set (tree6d, &query6d, &point_b.position, query_cache_element);
	ph6_query (tree6d, &query6d, &query_cache);

	printf ("boxes intersecting point %c (%.1f, %.1f, %.1f)\n", point_b.id, point_b.position.x, point_b.position.y, point_b.position.z);
	for (int iter = 0; iter < cvector_size (query_cache); iter++)
	{
		printf ("  box %c\n", query_cache[iter]->id);
	}
	printf ("\n");

	cvector_clear (query_cache);

	ph6_query_box_point_set (tree6d, &query6d, &point_a.position, query_cache_element);
	ph6_query (tree6d, &query6d, &query_cache);

	printf ("boxes intersecting point %c (%.1f, %.1f, %.1f)\n", point_a.id, point_a.position.x, point_a.position.y, point_a.position.z);
	for (int iter = 0; iter < cvector_size (query_cache); iter++)
	{
		printf ("  box %c\n", query_cache[iter]->id);
	}

	cvector_free (query_cache);
	ph6_clear (tree6d);
	free (tree6d);

	return 0;
}
