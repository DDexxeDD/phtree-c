#ifndef _print_h_
#define _print_h_

#include "phtree.h"

void print_point (phtree_point_t* point);
void node_point_print (phtree_node_t* node);
void entry_print_values (phtree_entry_t* entry);
void tree_print (phtree_t* tree);
void print_node (phtree_node_t* node, void* data);

#endif
