#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include "cvector.h"

#include "source/32bit/phtree32_2d.h"

/*
 * 2 dimensional spatial hash demo
 *
 * if you are familiar with spatial hashing
 * 	this is a version of it using a phtree
 * 
 * a 2d space is broken up into cells/buckets that are 32x32 units
 * the cells/buckets are the indices in the tree
 * 2d points have their xy positions divided by 32 and floored
 * 	to determine which cell/bucket they are put in
 */

typedef struct
{
	float x;
	float y;
} vector2_t;

typedef struct
{
	vector2_t position;
	int id;
} thing2d_t;

typedef struct
{
	int x;
	int y;
	// store things directly in the elements
	// if you are working with things outside of this spatial hash/index
	// 	you probably want to store references to things here instead of the actual things
	cvector (thing2d_t) things;
} element_2d_t;

phtree_key_t int32_to_key (int32_t input)
{
	phtree_key_t b = 0;

	memcpy (&b, &input, sizeof (phtree_key_t));
	b ^= PHTREE32_SIGN_BIT;  // flip sign bit

	return b;
}

void* element_2d_create (void* input)
{
	element_2d_t* new_element = calloc (1, sizeof (*new_element));

	if (!new_element)
	{
		return NULL;
	}

	thing2d_t* thing = input;

	new_element->x = floorf (thing->position.x / 32.0f);
	new_element->y = floorf (thing->position.y / 32.0f);
	cvector_init (new_element->things, 2, NULL);

	return new_element;
}

void element_2d_destroy (void* element_in)
{
	element_2d_t* element = (element_2d_t*) element_in;

	cvector_free (element->things);
	free (element);
}

ph2_point_t thing2d_to_point (thing2d_t* thing)
{
	int32_t x = floorf (thing->position.x / 32);
	int32_t y = floorf (thing->position.y / 32);
	ph2_point_t point;

	ph2_point_set (&point, int32_to_key (x), int32_to_key (y));

	return point;
}

void print_2d_thing (void* element_in, void* unused)
{
	element_2d_t* element = element_in;

	printf ("bucket (%i, %i)\n", element->x, element->y);

	for (int iter = 0; iter < cvector_size (element->things); iter++)
	{
		printf ("  thing %i: {%f, %f}\n", element->things[iter].id, element->things[iter].position.x, element->things[iter].position.y);
	}
}

void find_2d (ph2_t* tree, thing2d_t* thing_to_find)
{
	printf ("find thing at {%f, %f}\n", thing_to_find->position.x, thing_to_find->position.y);

	ph2_point_t point = thing2d_to_point (thing_to_find);
	element_2d_t* bucket = ph2_find (tree, &point);
	bool found = false;
	if (bucket)
	{
		thing2d_t* thing = NULL;
		for (int iter = 0; iter < cvector_size (bucket->things); iter++)
		{
			if (bucket->things[iter].position.x == thing_to_find->position.x
				&& bucket->things[iter].position.y == thing_to_find->position.y)
			{
				thing = &bucket->things[iter];
			}
		}

		if (thing)
		{
			printf ("  found thing %i at {%f, %f} in bucket (%i, %i)\n", thing->id, thing->position.x, thing->position.y, bucket->x, bucket->y);
			found = true;
		}
	}

	if (!found)
	{
		printf ("  thing at {%f, %f} not found!\n", thing_to_find->position.x, thing_to_find->position.y);
	}
}

void query_cache_2d (void* element_in, void* data)
{
	element_2d_t* element = element_in;
	cvector (element_2d_t*)* list = data;

	cvector_push_back (*list, element);
}

int main ()
{
	srand (time (NULL));

	ph2_t* tree = calloc (1, sizeof (*tree));
	ph2_initialize (
		tree,
		element_2d_create,
		element_2d_destroy,
		NULL,
		NULL,
		NULL);

	// keep a separate list of the things we are creating
	// 	so we can easily query a thing we know exists later
	cvector (thing2d_t) things = NULL;
	cvector_init (things, 10, NULL);

	int new_id = 0;

	// generate some points between bucket min (-2, -2) and bucket max (1, 1)
	for (int iter = 0; iter < 15; iter++)
	{
		float x = (float) (rand () % 128) + ((float) (rand () % 32) / 32.0f) - 64.0f;
		float y = (float) (rand () % 128) + ((float) (rand () % 32) / 32.0f) - 64.0f;
		thing2d_t thing = {{x, y}, new_id};
		cvector_push_back (things, thing);

		thing2d_t* new_thing = cvector_back (things);
		ph2_point_t point = thing2d_to_point (new_thing);
		element_2d_t* element = ph2_insert (tree, &point, new_thing);

		cvector_push_back (element->things, *new_thing);
		new_id++;
	}

	ph2_for_each (tree, print_2d_thing, NULL);

	printf ("\nfind some things we know exist\n");
	find_2d (tree, &things[4]);
	find_2d (tree, &things[7]);

	printf ("\nfind a thing which probably does not exist\n");
	find_2d (tree, &(thing2d_t) {{10.25f, 10.25f}, 0});

	printf ("\nfind a thing we know does not exist\n");
	find_2d (tree, &(thing2d_t) {{100.0f, 100.0f}, 0});

	printf ("\nremove thing %i\n", things[11].id);
	ph2_point_t remove_point = thing2d_to_point (&things[11]);
	ph2_remove (tree, &remove_point);
	printf ("find thing %i which we just removed\n", things[11].id);
	find_2d (tree, &things[11]);

	printf ("\nquery from {0.0, 0.0} to {64.0, 64.0}\n");
	ph2_point_t query_min = thing2d_to_point (&(thing2d_t) {{0.0f, 0.0f}, 0});
	ph2_point_t query_max = thing2d_to_point (&(thing2d_t) {{64.0f, 64.0f}, 0});
	ph2_query_t query;
	// set up a query for points in the +,+ quadrant
	// 	hopefully rand generated some points there :D
	ph2_query_set (tree, &query, &query_min, &query_max, query_cache_2d);

	// a cache for the elements our query finds
	// !! if the tree changes after the cache is created (insertions/removals) !!
	// 	expect your pointers to elements to be invalid
	// 	you should rerun your query
	cvector (element_2d_t*) elements = NULL;
	cvector_init (elements, 2, NULL);

	ph2_query (tree, &query, &elements);

	printf ("query results:\n");
	if (cvector_size (elements) == 0)
	{
		printf ("  nothing in query!\n");
	}
	for (int iter = 0; iter < cvector_size (elements); iter++)
	{
		printf ("  bucket (%i, %i)\n", elements[iter]->x, elements[iter]->y);

		for (int jter = 0; jter < cvector_size (elements[iter]->things); jter++)
		{
			thing2d_t* thing = &elements[iter]->things[jter];
			printf ("    thing %i at {%f, %f}\n", thing->id, thing->position.x, thing->position.y);
		}
	}

	cvector_free (elements);
	cvector_free (things);
	ph2_clear (tree);
	free (tree);

	return 0;
}
