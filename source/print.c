#include <stdio.h>
#include <stdint.h>

#include "print.h"

// redefining this here because it isnt in phtree.h anymore
#define node_is_leaf(node) ((node)->postfix_length == 0)

char* child_to_string (int child)
{
	switch (child)
	{
		case 0:
			return "00";
			break;
		case 1:
			return "01";
			break;
		case 2:
			return "10";
			break;
		case 3:
			return "11";
			break;
		default:
			return "xx";
			break;
	}
}

void key_to_binary_string (phtree_key_t number, char* string)
{
	int string_iter = (BIT_WIDTH) + (BIT_WIDTH / 8);

	if (BIT_WIDTH % 8 == 0)
	{
		string_iter--;
	}

	string[string_iter] = '\0';
	string_iter--;

	for (int iter = (BIT_WIDTH - 1); iter >= 0; iter--)
	{
		string[string_iter] = ((number >> ((BIT_WIDTH - 1) - iter)) & KEY_ONE) + '0';
		if (iter > 0 && iter % 8 == 0)
		{
			string_iter--;
			string[string_iter] = ' ';
		}

		string_iter--;
	}
}

void print_point (phtree_point_t* point)
{
	// big enough to hold a 64 bit number
	char string[72] = {0};

	key_to_binary_string (point->values[0], string);
	printf (" %s\n", string);
	key_to_binary_string (point->values[1], string);
	printf (" %s\n", string);
}

void node_point_print (phtree_node_t* node)
{
	// BIT_WIDTH + (BIT_WIDTH / 8) should cover the spaces and ending '\0' from key_to_binary_string
	char string[BIT_WIDTH + (BIT_WIDTH / 8)];
	key_to_binary_string (node->point.values[0], string);
	memset (&string[BIT_WIDTH - node->postfix_length], '-', node->postfix_length);
	string[BIT_WIDTH] = '\0';
	string[BIT_WIDTH - node->postfix_length - 1] = '|';
	printf ("%s\n", string);
	key_to_binary_string (node->point.values[1], string);
	memset (&string[BIT_WIDTH - node->postfix_length], '-', node->postfix_length);
	string[BIT_WIDTH] = '\0';
	string[BIT_WIDTH - node->postfix_length - 1] = '|';
	printf ("%s\n", string);
}

void entry_print_values (phtree_entry_t* entry)
{
	for (int iter = 0; iter < cvector_size (entry->elements); iter++)
	{
		printf ("%i, ", entry->elements[iter]);
	}
}

void tree_print_nodes (phtree_node_t* node)
{
	if (node_is_leaf (node))
	{
		printf ("node %c: postfix: %i\n", node->id, node->postfix_length);
		node_point_print (node);

		for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
		{
			if (node->children[iter])
			{
				phtree_entry_t* child = node->children[iter];

				printf ("  %s: ", child_to_string (iter));
				entry_print_values (child);
				printf ("\n");
			}
			else
			{
				printf ("  %s: null\n", child_to_string (iter));
			}
		}

		printf ("\n");

		return;
	}

	printf ("node %c: postfix: %i\n", node->id, node->postfix_length);
	node_point_print (node);
	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (node->children[iter])
		{
			printf ("  %s: %c\n", child_to_string (iter), ((phtree_node_t*)node->children[iter])->id);
		}
		else
		{
			printf ("  %s: null\n", child_to_string (iter));
		}
	}

	printf ("\n");
	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (node->children[iter])
		{
			tree_print_nodes (node->children[iter]);
		}
	}
}

void tree_print (phtree_t* tree)
{
	printf ("tree:\n");
	tree_print_nodes (&tree->root);
}

void print_node (phtree_node_t* node, void* data)
{
	if (!node)
	{
		return;
	}

	if (node_is_leaf (node))
	{
		printf ("node %c: postfix: %i\n", node->id, node->postfix_length);
		node_point_print (node);

		for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
		{
			if (node->children[iter])
			{
				phtree_entry_t* child = node->children[iter];

				printf ("  %s: ", child_to_string (iter));
				entry_print_values (child);
				printf ("\n");
			}
			else
			{
				printf ("  %s: null\n", child_to_string (iter));
			}
		}

		printf ("\n");

		return;
	}

	printf ("node %c: postfix: %i\n", node->id, node->postfix_length);
	node_point_print (node);
	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (node->children[iter])
		{
			printf ("  %s: %c\n", child_to_string (iter), ((phtree_node_t*)node->children[iter])->id);
		}
		else
		{
			printf ("  %s: null\n", child_to_string (iter));
		}
	}
}

