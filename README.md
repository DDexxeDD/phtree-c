# PH-Tree C

A C implementation of a [PH-Tree](https://tzaeschke.github.io/phtree-site/).  A PH-Tree is a spatial indexing data structure like a quadtree or octree.

This is a single map implementation which supports 8, 16, 32, or 64 bit widths.  Being a single map means only 1 element is stored at each index.  However, you define what an element is, so you can store as much and whatever data you want in an element (such as collections, which will turn this into a multi map).

The dimensionality of the phtrees here are hardcoded.  This library supports 1-6 dimensions.  Higher dimensionality is possible with ph-trees, but this library is not suited for them.  Higher dimensionality than 6 requires internal structures and systems which this implementation does not have.  Attempting to generate phtrees with more than 6 dimensions will create broken code, do not generate phtrees with more than 6 dimenions. 

This library is designed to be fairly generic, which means there are a lot of void pointers being passed around.  Pay attention to what you are passing in to phtree functions and how you cast what you get out.

For this library to work you _must_ define a few things.
1. The structure/element you want to store at each index
2. A function for creating/allocating one of those elements
3. A function for destroying/deallocating one of those elements
4. A function for converting a user defined value to a `phtree_key_t`, generic converters for ints, floats, and doubles are provided
5. A function for converting a user defined type to a `phtree_point_t`
6. Function(s) for what to do when querying or iterating the tree.

***Check the [examples](https://github.com/DDexxeDD/phtree-c/tree/main/examples) for demonstsrations of all of this.***

If you are interested in a simple implementation as reference for understanding or implementing your own phtree, check out the '[reference](https://github.com/DDexxeDD/phtree-c/releases/tag/reference)' tag.


## Reporting Bugs/Contributing/Contact

If you catch a bug, think the design of this library could or should be different, or would like to see some different/better documentation, please create an issue!

When filing an issue, do not worry about the template files (unless your issue is directly related to the template files), just note the issue(s) in whatever .c or .h file you are using.

There is also a [PH-Tree Discord](https://discord.gg/YmJTWYHPCA) server if you are interested.


## About the Design

Why templates and a bunch of bit/dimensionality specific files?  Because supporting arbitrary bit widths and dimensionality in a single data structure would require much more complexity and overhead.  You probably know what you need/want for your project, all of that complexity and overhead isn't going to help you any, its just going to slow you down.  So the complexity is rolled into the templates and you get a tighter, more efficient, more hackable tree for your project.

Please note that while internal tree data types are publicly defined, you **should not** be directly touching anything inside of them, unless you are changing the functionality of the tree.  They are public because this is C and if you want to shoot yourself in the foot, go for it, its your life :D

Best practice for using this library is probably to wrap most/all of the tree functionality with your own functions, the demos do this with most functions... pay attention to the demos.


## Simple Usage

The phtree source files are in the `source` folder.

1. Choose a bit width (8, 16, 32, 64).  Unless you are indexing doubles, you will probably be fine with 32 bit.
2. Choose the dimensionality that you want (1-6).
3. Add the chosen .h and .c to your project.
4. Add the `phtreeXX_common.h` and `phtreeXX_common.c` to your project, where XX is your chosen bit width.

You can have any combination of dimensionalities of the same bit width in your project, they wil not conflict with eachother.  Howerver, you can not have trees with different bit width of the same dimensions at the same time, as they use the same type names.  If you want to use different bit width trees of the same dimensionality see [Advanced Usage](#advanced-usage) below.

[demo_1d](https://github.com/DDexxeDD/phtree-c/tree/blob/main/examples/demo_1d.c) is an example of a simple binary tree.

[demo_2d_simple](https://github.com/DDexxeDD/phtree-c/tree/blob/main/examples/demo_2d_simple.c) is an example of indexing 2d points.

[demo_2d_spatial_hash](https://github.com/DDexxeDD/phtree-c/tree/blob/main/examples/demo_2d_spatial_hash.c) is an example of indexing 2d points in a spatial hash.  Points in the tree are buckets of size 32x32 which points are stored in.

[demo_3d6d](https://github.com/DDexxeDD/phtree-c/tree/blob/main/examples/demo_2d_spatial_hash.c) is an example of indexing both 3d points and 3d boxes.  This is a demonstration of using trees of higher dimensions to store boxes in lower dimensions.


### Define an element to be stored in the tree.

```
typedef struct
{
  ...
} my_element;
```

The tree will use user defined functions to allocate and deallocate these elements as necessary.

### Define a function to allocate and intialize an element you want to store in the tree

`void* element_create (void* input);`

This function will need to allocate and initialize an element.  It should return a pointer to the allocated element.

When inserting something in to the tree which creates a new element, input will be whatever you passed in to the insert function.  See the [2d spatial hash demo](https://github.com/DDexxeDD/phtree-c/tree/blob/main/examples/demo_2d_spatial_hash.c) for an example of this being used.  If initializing your element requires more data than the initial object you are creating it for has, you will need to use the pointer returned by the insert function to finish initialization.

### Define a function to deallocate an element

`void element_destory (void* element);`

This function will need to deallocate anything in the element that needs to be deallocated, as well as deallocate the element itself.

### Define a function to convert a single value to a phtree_key_t

`phtree_key_t convert_to_key (void* input);`

This function needs to convert whatever is passed in to it (likely an integer or float), into a single phtree_key_t value.

### Define a function to convert a more complex type into a tree point

`void convert_to_point (tree_t* tree, point* out, void* input)`

(tree and point will be the types specific to your chosen dimensionality)
This function needs to convert whatever your indexes are based on, into a point in the tree.  You should use your chosen tree's point_set function to set the 'out' argument.  The point_set function internally calls the tree's convert_to_key function on each of the values passed in to it, so you do not need to convert values in the convert_to_point function.

### (Optional) Define a function to convert a user defined type into a tree point of half dimensions

`void convert_to_box_point (tree_t* tree, point* out, void* input)`

See [Indexing Axis Aligned Boxes](#indexing-axis-aligned-boxes) for an explanation of using phtrees for boxes.

If you are using a tree with an even number of dimensions to represent boxes of lower dimensions, and you want to query those boxes, you will need this function.  See [demo_3d6d](https://github.com/DDexxeDD/phtree-c/tree/blob/main/examples/demo_2d_spatial_hash.c) for a demonstration of this.

### Define iteration functions for iterating or querying the tree

Unless you only ever need single elements from the tree, you will need to define functions for iterating the entire tree with the for_each function, or iterating elements with window queries.  You will need to define a phtree_iteration_function_t.  An iteration function takes an element to be worked on and a pointer to any outside data you want to pass in to the function.  If you want to cache elements found in a query, pass the structure you want to cache elements in as the data argument, and put the elements in that structure using the iteration function.


## Advanced Usage

The .h an .c files are generated using [mustache](https://mustache.github.io/) templates.  If you want to customize the function prefix (ph1_, ph2_, ph3_, ...) or the dimensions of a tree, you will need to write a (or modify an existing) mustache hash file.  The templates and existing mustache hash files are located in the `templates` directory.

***!! Do not attempt to generate trees with more than 6 dimensions !!***

The code can not handle more than 6 dimensions and will break.

If you want to be able to use trees of different bit widths and the same dimensionality at the same time, you will need to generate them with different prefixes (example: ph32_1_, ph64_1_)


## Indexing Axis Aligned Boxes

All bit widths and dimensionalities index single points.  However, it is possible to index axis aligned boxes using single points, if you use a tree with twice as many dimenions as the space you want to work in.

For example, if you are working in 2 dimensional space and have a box with minimum point (2, 3) and maximum point (4, 5).  You can index that box in a 4 dimensional tree as the single point (2, 3, 4, 5).

Similarly to that 2 dimensional example, 1 dimensional line segments can be indexed in 2 dimensional trees, and 3 dimensional cuboids can be indexed in 6 dimensional trees.

When you are indexing lower dimensional boxes as points in higher dimensional trees you will need to use 'box' queries instead of regular 'window' queries, which are explained below.


## Queries

There are two types of queries in this implementation, 'window' and 'box' queries.  You will notice that there is only 1 query type, and only 1 query function.  The difference between the two types is in how a query is constructed, and when a query is used.

Window queries are normal point to point range queries.  In a window query you set a lower point bound and an upper point bound and find all the points contained in the bounding window.

Box queries are used in even dimensional trees in which points are being used to represent lower dimensional bounding boxes.  If you are using a 6d tree to index 3d cuboids, you would use a box query to find those points.  The same goes for using 4d trees to index 2d boxes and 2d trees to index 1d line segments.


## A Note About Node Size and Memory Alignment

On the development machine, in almost all combinations of bit widths and dimensions, nodes in the tree are less than 64 bytes in size.  Only trees of 64 bit width and 6 dimensions are greater than 64 bytes.  So if you are worried about cache lines, don't use 64 bit width and 6 dimenions.

In trees with low bit widths and dimensions, the node point will align to the node's children pointer, so there is a lower limit on how small you can make nodes, unless you disable memory alignment.


## Building the examples

You will need [meson](https://mesonbuild.com/Getting-meson.html) and [ninja](https://ninja-build.org/) to build the examples.

**Build**

```
meson setup build
meson compile -C build
```

This will create the '`build`' directory.
The executables will be in the `build` directory.

The demos have only been tested on linux, but they should work on other platforms.


## Licenses

Some of the basic tree code was was directly derived from the [improbable](https://github.com/improbable-eng/phtree-cpp) implementation, which is licensed under Apache 2.0.  I don't care what you do with any of my code, but instead of trying to separate it all out, this is also licensed under Apache 2.0.

The cvector license is in the `examples/external/cvector` directory.
