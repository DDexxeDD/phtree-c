#!/bin/sh

# using mustach from https://gitlab.com/jobol/mustach
mustach templates/mustache_hash_32_1d.json templates/phtree_header_template.h > source/32bit/phtree_1d.h
mustach templates/mustache_hash_32_1d.json templates/phtree_source_template.c > source/32bit/phtree_1d.c

mustach templates/mustache_hash_32_2d.json templates/phtree_header_template.h > source/32bit/phtree_2d.h
mustach templates/mustache_hash_32_2d.json templates/phtree_source_template.c > source/32bit/phtree_2d.c

mustach templates/mustache_hash_32_3d.json templates/phtree_header_template.h > source/32bit/phtree_3d.h
mustach templates/mustache_hash_32_3d.json templates/phtree_source_template.c > source/32bit/phtree_3d.c

mustach templates/mustache_hash_64_1d.json templates/phtree_header_template.h > source/64bit/phtree_1d.h
mustach templates/mustache_hash_64_1d.json templates/phtree_source_template.c > source/64bit/phtree_1d.c

mustach templates/mustache_hash_64_2d.json templates/phtree_header_template.h > source/64bit/phtree_2d.h
mustach templates/mustache_hash_64_2d.json templates/phtree_source_template.c > source/64bit/phtree_2d.c

mustach templates/mustache_hash_64_3d.json templates/phtree_header_template.h > source/64bit/phtree_3d.h
mustach templates/mustache_hash_64_3d.json templates/phtree_source_template.c > source/64bit/phtree_3d.c
