# PH-Tree C Implementation

A C implementation of a [PH-Tree](https://tzaeschke.github.io/phtree-site/).  Visit that website for an explanation of PH-Trees and for links to other implementations.


## About

This is a simple implementation I did for myself, but I have tried to document it well enough for anyone interested in implementing their own PH-Tree.

The relevant source code is in `phtree.h` and `phtree.c`.  The other source files are just used for demonstration purposes.

This implementation is a _multimap_ version, storing more than 1 element per entry in the tree is handled with the [cvector](https://github.com/eteran/c-vector) library.  Window queries also store their results in cvectors.

If you want to experiment with different bit widths or dimensionality, change BIT_WIDTH and/or DIMENSIONS in `phtree.h`.  This implementation uses plain arrays to store dimensional keys, so while it _can_ handle 4+ dimensions, setting DIMENSIONS to 4+ is not recommended.  For explanations about how to handle more than 3 dimenions, please refer to the PH-Tree [website](https://tzaeschke.github.io/phtree-site/).


## Building

You will need [meson](https://mesonbuild.com/Getting-meson.html) and [ninja](https://ninja-build.org/) to build this project.

**Build**

```
meson setup build
meson compile -C build
```

This will create the '`build`' directory.
The executable `phtree` will be in the `build` directory.

This was only tested on linux, so no idea if it works properly on anything else.


## Running

```
./build/phtree
```

## Licenses

Some of this code was was diretly derived from the [improbable](https://github.com/improbable-eng/phtree-cpp) implementation, which is licensed under Apache 2.0.  I don't care what you do with any of my code, but instead of trying to separate it all out, this is also licensed under Apache 2.0.

The cvector license is in the `external/cvector` directory.
