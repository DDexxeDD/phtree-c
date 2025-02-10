# PH-Tree C

A C implementation of a [PH-Tree](https://tzaeschke.github.io/phtree-site/).  A PH-Tree is a spatial indexing data structure like an r-tree or a kd-tree.

This is a 32 or 64 bit, single map implementation.  That means only 1 element is stored at each index.  However, you define what an element is, so you can store as much and whatever data you want in an element (such as collections, which will turn this into a multi map).

The dimensionality of the phtrees here are hardcoded.  This library supports 1, 2, or 3 dimensions.  Higher dimensionality is possible with ph-trees, but this library is not suited for them.  Because this library stores pointers to children in a plain array, anything more than 3 dimensions will become very expensive in memory.

This library is designed to be fairly generic, which means there are a lot of void pointers being passed around.  Pay attention to what you are passing in to phtree functions and how you cast what you get out.

For this library to work you _must_ define a few things.
1. The structure/element you want to store at each index
2. A function for creating/allocating one of those elements
3. A function for destroying/deallocating one of those elements
4. A function for converting a user defined value to a phtree_key_t (uint64_t), generic converters for int64_t and double are provided
5. A function for converting a user defined type to a phtree_point_t (probably a vector2 or vector3)
6. Function(s) for what to do when querying or iterating the tree.

The example demonstrates all of these necessary definitions.


## Contributing

If you catch a bug, think the design of this library could or should be different, or would like to see some different/better documentation, please create an issue!


## Simple Usage

Add the .h and .c of the phtree bit width and dimensionality you want, to your source tree.  You can have all 3 dimensionalities if you want, they do no conflict with eachother.  Include the .h file(s) wherever you want to have a tree.  You can not have 32 and 64 bit trees of the same dimensions at the same time as they use the same type names.  If you want to use 32 and 64 bit trees of the same dimensionality see [Advanced Usage](#advanced-usage) below.

### Define an element to be stored in the tree.

```
typedef struct
{
  ...
} my_element;
```

The tree will use user defined functions to allocate and deallocate these elements as necessary.

### Define a function to allocate an element you want to store in the tree

`void* element_create ();`

This function will need to allocate and initialize an element.  It should return a pointer to the allocated element.

### Define a function to deallocate an element

`void element_destory (void* element);`

This function will need to deallocate anything in the element that needs to be deallocated, as well as deallocate the element itself.

### Define a function to convert a single value to a phtree_key_t

`phtree_key_t convert_to_key (void* input);`

This function needs to convert whatever is passed in to it (likely an integer or float), into a single phtree_key_t value.

### Define a function to convert a more complex type into a tree point

`void convert_to_point (tree_t* tree, point* out, void* index)`

(tree and point will be the types specific to your chosen dimensionality)
This function needs to convert whatever your indexes are based on, into a point in the tree.  You should use your chosen tree's point_set function to set the 'out' argument.  The point_set function internally calls the tree's convert_to_key function on each of the values passed in to it, so you do not need to convert values in the convert_to_point function.

### Define iteration functions for iterating or querying the tree

Unless you only ever need single elements from the tree, you will need to define functions for iterating the entire tree with the for_each function, or iterating elements with window queries.  You will need to define a phtree_iteration_function_t.  An iteration function takes an element to be worked on and a pointer to any outside data you want to pass in to the function.  If you want to cache elements found in a query, pass the structure you want to cache elements in as the data argument, and put the elements in that structure using the iteration function.


## Advanced Usage

The .h an .c files are generated using [mustache](https://mustache.github.io/) templates.  If you want to customize the function prefix (ph1_, ph2_, ph3_) or the dimensions of a tree, you will need to write a (or modify an existing) mustache hash file.  The templates and existing mustache hash files are located in the `templates` directory.

If you want a tree with more than 3 dimensions, you will also have to alter the templates to accomodate them.  Specifically the point_set function.

If you want to be able to use 32 and 64 bit trees of the same dimensionality at the same time, you will need to generate them with different prefixes (example: ph32_1_, ph64_1_)


## Building the example

You will need [meson](https://mesonbuild.com/Getting-meson.html) and [ninja](https://ninja-build.org/) to build this project.

**Build**

```
meson setup build
meson compile -C build
```

This will create the '`build`' directory.
The executable `phtree` will be in the `build` directory.

This was only tested on linux, so I have no idea if it works properly on anything else.


## Running

```
./build/phtree
```

## Licenses

Some of this code was was diretly derived from the [improbable](https://github.com/improbable-eng/phtree-cpp) implementation, which is licensed under Apache 2.0.  I don't care what you do with any of my code, but instead of trying to separate it all out, this is also licensed under Apache 2.0.

The cvector license is in the `example/external/cvector` directory.
