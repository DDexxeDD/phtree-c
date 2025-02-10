#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cvector.h"

#include "phtree32_2d.h"

typedef struct
{
	int x;
	int y;
} vector2_t;

typedef struct
{
	vector2_t position;
	int id;
} thing_t;

typedef struct
{
	int number;
} element_simple_t;

typedef struct
{
	int id;
	cvector (int) numbers;
} element_complex_t;

/*
 * !! be very careful about how you handle input here !!
 * 	make sure you cast and dereference input to the type it actually is (here an int)
 */
phtree_key_t convert_to_key (void* input)
{
	return phtree_int32_to_key (*(int*) input);
}

void convert_to_point (ph2_t* tree, ph2_point_t* out, void* index)
{
	thing_t* thing = index;
	ph2_point_set (tree, out, &thing->position.x, &thing->position.y);
}

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

void* element_complex_create ()
{
	element_complex_t* new_element = calloc (1, sizeof (*new_element));

	if (!new_element)
	{
		return NULL;
	}

	cvector_init (new_element->numbers, 2, NULL);

	if (!new_element->numbers)
	{
		free (new_element);

		return NULL;
	}

	new_element->id = -1;

	return new_element;
}

void element_complex_destroy (void* element_in)
{
	element_complex_t* element = element_in;

	cvector_free (element->numbers);
	free (element);
}

element_complex_t* complex_insert (ph2_t* tree, thing_t* thing)
{
	element_complex_t* element = ph2_insert (tree, thing);

	if (!element)
	{
		return NULL;
	}

	// we only want unique ids in our number list
	for (int iter = 0; iter < cvector_size (element->numbers); iter++)
	{
		if (element->numbers[iter] == thing->id)
		{
			return element;
		}
	}

	cvector_push_back (element->numbers, thing->id);

	return element;
}

void complex_remove (ph2_t* tree, thing_t* thing)
{
	element_complex_t* element = ph2_find (tree, thing);

	for (int iter = 0; iter < cvector_size (element->numbers); iter++)
	{
		if (element->numbers[iter] == thing->id)
		{
			cvector_erase (element->numbers, iter);
			break;
		}
	}

	// if the element is empty
	// 	remove the element
	if (cvector_size (element->numbers) == 0)
	{
		ph2_remove (tree, thing);
	}
}

void simple_element_print (void* element_in, void* unused)
{
	element_simple_t* element = element_in;

	printf ("%i, ", element->number);
}

void complex_element_print (void* element_in, void* unused)
{
	element_complex_t* element = element_in;

	printf ("complex element %i: ", element->id);
	for (int iter = 0; iter < cvector_size (element->numbers); iter++)
	{
		printf ("%i, ", element->numbers[iter]);
	}
	printf ("\n");
}

void simple_map_query (void* element_in, void* data)
{
	element_simple_t* element = element_in;
	cvector (element_simple_t*)* list = data;

	cvector_push_back (*list, element);
}

int main ()
{
	/*
	 * simple map
	 */
	ph2_t* simple_tree = ph2_create (
		element_simple_create,
		element_simple_destroy,
		convert_to_key,
		convert_to_point);

	thing_t things[9];
	int new_id = 0;

	printf ("simple map:\n");

	for (int xter = 0; xter < 3; xter++)
	{
		for (int yter = 0; yter < 3; yter++)
		{
			things[new_id] = (thing_t) {(vector2_t) {xter * 25, yter * 25}, new_id};
			printf ("inserting thing %i at position (%i, %i)\n", things[new_id].id, things[new_id].position.x, things[new_id].position.y);
			element_simple_t* element = ph2_insert (simple_tree, &things[new_id]);
			element->number = things[new_id].id;
			new_id++;
		}
	}

	printf ("\nthings in map: ");
	ph2_for_each (simple_tree, simple_element_print, NULL);
	printf ("\n\n");

	ph2_remove (simple_tree, &things[5]);
	ph2_remove (simple_tree, &things[6]);

	printf ("removed things 5 and 6\n");
	printf ("elements in map: ");
	ph2_for_each (simple_tree, simple_element_print, NULL);
	printf ("\n\n");

	cvector (element_simple_t*) query_things = NULL;
	cvector_init (query_things, 2, NULL);

	ph2_window_query_t* query = ph2_query_create ();
	ph2_query_set (simple_tree, query, &(thing_t) {{0, 1}}, &(thing_t) {{25, 51}}, simple_map_query);
	ph2_query (simple_tree, query, &query_things);

	printf ("querying elements from (0, 1) to (25, 51)\n");
	printf ("elements in query: ");
	for (int iter = 0; iter < cvector_size (query_things); iter++)
	{
		printf ("%i, ", query_things[iter]->number);
	}

	printf ("\n\n");

	free (query);
	cvector_free (query_things);
	ph2_free (simple_tree);

	/*
	 * complex map
	 */
	ph2_t* complex_tree = ph2_create (
		element_complex_create,
		element_complex_destroy,
		convert_to_key,
		convert_to_point);

	int element_id = 0;
	for (int iter = 0; iter < 9; iter++)
	{
		if (iter < 4)
		{
			things[iter].position.x = 5;
			things[iter].position.y = 5;
		}
		else
		{
			things[iter].position.x = 10;
			things[iter].position.y = 10;
		}

		element_complex_t* element = complex_insert (complex_tree, &things[iter]);
		if (element->id < 0)
		{
			element->id = element_id;
			element_id++;
		}
	}

	printf ("\ncomplex map:\n");
	ph2_for_each (complex_tree, complex_element_print, NULL);
	printf ("\n");

	complex_remove (complex_tree, &things[2]);
	complex_remove (complex_tree, &things[7]);

	printf ("removed things 2 and 7\n");
	ph2_for_each (complex_tree, complex_element_print, NULL);
	printf ("\n");

	complex_remove (complex_tree, &things[0]);
	complex_remove (complex_tree, &things[1]);
	complex_remove (complex_tree, &things[3]);

	printf ("removed things 0, 1, and 3, which will remove element 0\n");
	ph2_for_each (complex_tree, complex_element_print, NULL);

	ph2_free (complex_tree);

	return 0;
}
