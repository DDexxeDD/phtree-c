#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "cvector.h"

#include "source/32bit/phtree32_2d.h"

/*
 * simple 2 dimensional demo
 * 
 * store some numbers directly in the tree
 */

typedef struct
{
	int x;
	int y;
	int id;
} thing2d_t;

typedef struct
{
	int x;
	int y;
	int number;
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

	new_element->x = thing->x;
	new_element->y = thing->y;
	new_element->number = thing->id;

	return new_element;
}

void element_2d_destroy (void* element)
{
	free (element);
}

ph2_point_t thing2d_to_point (thing2d_t* thing)
{
	ph2_point_t point;
	ph2_point_set (&point, int32_to_key (thing->x), int32_to_key (thing->y));

	return point;
}

element_2d_t* tree_insert_thing2d (ph2_t* tree, thing2d_t* thing)
{
	ph2_point_t point = thing2d_to_point (thing);
	return ph2_insert (tree, &point, thing);
}

void print_2d_thing (void* element_in, void* unused)
{
	element_2d_t* element = element_in;

	printf ("  element %i at (%i, %i)\n", element->number, element->x, element->y);
}

void find_2d (ph2_t* tree, thing2d_t* thing_to_find)
{
	printf ("find thing at (%i, %i)\n", thing_to_find->x, thing_to_find->y);

	ph2_point_t point = thing2d_to_point (thing_to_find);
	element_2d_t* element = ph2_find (tree, &point);

	if (element)
	{
		printf ("  found element %i at (%i, %i)\n", element->number, element->x, element->y);
	}
	else
	{
		printf ("  element at (%i, %i) not found!\n", thing_to_find->x, thing_to_find->y);
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
	// 	so we can easily query things we know exist later
	thing2d_t things[15] = {0};
	int new_id = 0;

	// generate some points between bucket min (-2, -2) and bucket max (1, 1)
	for (int iter = 0; iter < 15; iter++)
	{
		things[iter].x = (rand () % 128) - 64;
		things[iter].y = (rand () % 128) - 64;
		things[iter].id = new_id;

		tree_insert_thing2d (tree, &things[iter]);

		new_id++;
	}

	printf ("elements in the tree:\n");
	ph2_for_each (tree, print_2d_thing, NULL);

	printf ("\nfind some things we know exist\n");
	find_2d (tree, &things[4]);
	find_2d (tree, &things[7]);

	printf ("\nfind a thing which probably does not exist\n");
	find_2d (tree, &(thing2d_t) {10, 10, 0});

	printf ("\nfind a thing we know does not exist\n");
	find_2d (tree, &(thing2d_t) {100, 100, 0});

	printf ("\nremove thing %i\n", things[11].id);
	ph2_point_t remove_point = thing2d_to_point (&things[11]);
	ph2_remove (tree, &remove_point);
	printf ("find thing %i which we just removed\n", things[11].id);
	find_2d (tree, &things[11]);

	printf ("\nquery from (0, 0) to (64, 64)\n");
	ph2_point_t min = thing2d_to_point (&(thing2d_t) {0, 0, 0});
	ph2_point_t max = thing2d_to_point (&(thing2d_t) {64, 64, 0});
	ph2_query_t query;
	// set up a query for points in the +,+ quadrant
	// 	hopefully rand generated some points there :D
	ph2_query_set (tree, &query, &min, &max, query_cache_2d);

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
		printf ("  element %i at (%i, %i)\n", elements[iter]->number, elements[iter]->x, elements[iter]->y);
	}

	cvector_free (elements);
	ph2_clear (tree);
	free (tree);

	return 0;
}
