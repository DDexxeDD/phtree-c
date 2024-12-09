#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cvector.h"

#include "phtree.h"
#include "print.h"

int main ()
{
	phtree_t* tree;

	phtree_point_t* points[8];
	points[0] = point_create (1, 1);
	points[1] = point_create (2, 2);
	points[2] = point_create (3, 3);
	points[3] = point_create (4, 4);
	points[4] = point_create (1, 2);
	points[5] = point_create (-2, -2);
	points[6] = point_create (3, 3);
	points[7] = point_create (3, 3);

	tree = tree_create ();

	for (int iter = 0; iter < 8; iter++)
	{
		tree_insert (tree, points[iter], iter);
	}

	printf ("\n");

	tree_print (tree);

	phtree_entry_t* result = tree_find (tree, points[0]);
	if (result)
	{
		printf ("values at {1, 1}: ");
		entry_print_values (result);
		printf ("\n");
	}
	else
	{
		printf ("no values at {1, 1}\n");
	}

	printf ("\n*** removing entry at point 3 ***\n");
	tree_remove (tree, points[3]);

	phtree_point_t min;
	phtree_point_t max;
	point_set (&min, 2, 2);
	point_set (&max, 4, 4);
	phtree_window_query_t* query = window_query_create (min, max);

	tree_query_window (tree, query);

	printf ("\n*** window query ***\n");
	for (int iter = 0; iter < cvector_size (query->entries); iter++)
	{
		printf ("  {%i, %i}:\n    ", query->entries[iter]->point.values[0], query->entries[iter]->point.values[1]);
		entry_print_values (query->entries[iter]);
		printf ("\n");
	}
	printf ("\n");

	window_query_free (query);

	for (int iter = 0; iter < 8; iter++)
	{
		free (points[iter]);
	}

	tree_clear (tree);
	free (tree);

	return 0;
}
