#include <stdio.h>
#include <stdlib.h>

#include "cvector.h"

#include "source/32bit/phtree32_1d.h"

typedef struct
{
	int number;
} element_simple_t;

void* element_simple_create ()
{
	element_simple_t* new_element = calloc (1, sizeof (*new_element));

	if (!new_element)
	{
		return NULL;
	}

	new_element->number = -1;

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
void convert_to_point_1d (ph1_t* tree, ph1_point_t* out, void* input)
{
	ph1_point_set (tree, out, input);
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

void demo_1d_remove (ph1_t* tree, int index)
{
	printf ("removing %i\n", index);
	ph1_remove (tree, &index);
}

void demo_1d_find (ph1_t* tree, int index)
{
	element_simple_t* element = ph1_find (tree, &index);

	printf ("find %i\n", index);

	if (!element)
	{
		printf ("%i not in tree\n", index);
		return;
	}

	printf ("found %i\n", element->number);
}

// phtree_int32_to_key expects input to be a pointer to a signed 32 bit integer
phtree_key_t phtree_int32_to_key (void* input)
{
	phtree_key_t b = 0;

	memcpy (&b, input, sizeof (phtree_key_t));
	b ^= PHTREE32_SIGN_BIT;  // flip sign bit

	return b;
}

int main ()
{
	ph1_t* tree = ph1_create (
		element_simple_create,
		element_simple_destroy,
		phtree_int32_to_key,  // you can use the provided int32_to_key function directly for convert_to_key
		convert_to_point_1d);

	for (int32_t iter = -10; iter <= 10; iter += 2)
	{
		element_simple_t* element = ph1_insert (tree, &iter);

		printf ("inserting %i\n", iter);
		element->number = iter;
	}

	demo_1d_print (tree);

	demo_1d_remove (tree, 2);
	demo_1d_remove (tree, -8);
	demo_1d_print (tree);

	demo_1d_find (tree, -10);
	demo_1d_find (tree, 20);

	int query_min = -5;
	int query_max = 5;
	ph1_query_t query;
	ph1_query_set (tree, &query, &query_min, &query_max, query_cache_1d);

	// a cache for the elements our query finds
	// !! if the tree changes after the cache is created (insertions/removals) !!
	// 	expect your pointers to elements to be invalid
	// 	you should rerun your query
	cvector (int) integers = NULL;
	cvector_init (integers, 5, NULL);

	ph1_query (tree, &query, &integers);

	printf ("\nquerying the tree from %i to %i\n", query_min, query_max);
	printf ("elements in query: ");
	for (int iter = 0; iter < cvector_size (integers); iter++)
	{
		printf ("%i, ", integers[iter]);
	}

	printf ("\n");

	cvector_free (integers);
	ph1_free (tree);

	return 0;
}
