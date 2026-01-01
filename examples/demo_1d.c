#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cvector.h"

#include "source/32bit/phtree32_1d.h"

typedef struct
{
	int number;
} element_simple_t;

phtree_key_t int32_to_phtree_key (int32_t input)
{
	phtree_key_t b = 0;

	memcpy (&b, &input, sizeof (phtree_key_t));
	b ^= PHTREE32_SIGN_BIT;  // flip sign bit

	return b;
}

void* element_simple_create (void* input)
{
	element_simple_t* new_element = calloc (1, sizeof (*new_element));

	if (!new_element)
	{
		return NULL;
	}

	int32_t* value = input;
	new_element->number = *value;

	return new_element;
}

void element_simple_destroy (void* element)
{
	free (element);
}

void simple_element_print (void* element_in, void* unused)
{
	element_simple_t* element = element_in;

	printf ("%i, ", element->number);
}

void simple_map_query (void* element_in, void* data)
{
	element_simple_t* element = element_in;
	cvector (element_simple_t*)* list = data;

	cvector_push_back (*list, element);
}

/*
 * 1 dimensional demo
 *
 * points on a line
 */
ph1_point_t int32_to_point (int32_t input)
{
	ph1_point_t point;
	ph1_point_set (&point, int32_to_phtree_key (input));

	return point;
}

element_simple_t* tree_insert_int32 (ph1_t* tree, int32_t value)
{
	ph1_point_t point = int32_to_point (value);
	return ph1_insert (tree, &point, &value);
}

void query_cache_1d (void* element_in, void* data)
{
	element_simple_t* element = element_in;
	cvector (int)* list = data;

	cvector_push_back (*list, element->number);
}

void demo_1d_print (ph1_t* tree)
{
	printf ("elements in tree: ");
	ph1_for_each (tree, simple_element_print, NULL);
	printf ("\n\n");
}

void demo_1d_remove (ph1_t* tree, int32_t index)
{
	printf ("removing %i\n", index);
	ph1_point_t point = int32_to_point (index);
	ph1_remove (tree, &point);
}

void demo_1d_find (ph1_t* tree, int32_t index)
{
	ph1_point_t point = int32_to_point (index);
	element_simple_t* element = ph1_find (tree, &point);

	printf ("find %i\n", index);

	if (!element)
	{
		printf ("%i not in tree\n", index);
		return;
	}

	printf ("found %i\n", element->number);
}

int main ()
{
	ph1_t* tree = calloc (1, sizeof (*tree));
	ph1_initialize (
		tree,
		element_simple_create,
		element_simple_destroy,
		malloc,
		realloc,
		free);

	for (int32_t iter = -10; iter <= 10; iter += 2)
	{
		//ph1_point_t point = int32_to_point (iter);
		//element_simple_t* element = ph1_insert (tree, &point);
		tree_insert_int32 (tree, iter);

		printf ("inserting %i\n", iter);
	}

	demo_1d_print (tree);

	demo_1d_remove (tree, 2);
	demo_1d_remove (tree, -8);
	demo_1d_print (tree);

	demo_1d_find (tree, -10);
	demo_1d_find (tree, 20);

	int32_t min = -5;
	int32_t max = 5;
	ph1_point_t query_min = int32_to_point (min);
	ph1_point_t query_max = int32_to_point (max);
	ph1_query_t query;
	ph1_query_set (tree, &query, &query_min, &query_max, query_cache_1d);

	// a cache for the elements our query finds
	// !! if the tree changes after the cache is created (insertions/removals) !!
	// 	expect your pointers to elements to be invalid
	// 	you should rerun your query
	cvector (int) integers = NULL;
	cvector_init (integers, 5, NULL);

	ph1_query (tree, &query, &integers);

	printf ("\nquerying the tree from %i to %i\n", min, max);
	printf ("elements in query: ");
	for (int iter = 0; iter < cvector_size (integers); iter++)
	{
		printf ("%i, ", integers[iter]);
	}

	printf ("\n");

	cvector_free (integers);
	ph1_clear (tree);
	free (tree);

	return 0;
}
