#!/bin/sh

# using mustach from https://gitlab.com/jobol/mustach
mustach templates/mustache_hash_32_1d.json templates/phtree_header_template.h > source/phtree32_1d.h
mustach templates/mustache_hash_32_1d.json templates/phtree_source_template.c > source/phtree32_1d.c

mustach templates/mustache_hash_32_2d.json templates/phtree_header_template.h > source/phtree32_2d.h
mustach templates/mustache_hash_32_2d.json templates/phtree_source_template.c > source/phtree32_2d.c

mustach templates/mustache_hash_32_3d.json templates/phtree_header_template.h > source/phtree32_3d.h
mustach templates/mustache_hash_32_3d.json templates/phtree_source_template.c > source/phtree32_3d.c

mustach templates/mustache_hash_64_1d.json templates/phtree_header_template.h > source/phtree64_1d.h
mustach templates/mustache_hash_64_1d.json templates/phtree_source_template.c > source/phtree64_1d.c

mustach templates/mustache_hash_64_2d.json templates/phtree_header_template.h > source/phtree64_2d.h
mustach templates/mustache_hash_64_2d.json templates/phtree_source_template.c > source/phtree64_2d.c

mustach templates/mustache_hash_64_3d.json templates/phtree_header_template.h > source/phtree64_3d.h
mustach templates/mustache_hash_64_3d.json templates/phtree_source_template.c > source/phtree64_3d.c
