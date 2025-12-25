#!/bin/sh

# make sure we want to generate new source files
# because doing so will erase any changes made in existing source files
echo "Make sure any changes you wish to keep in existing source files"
echo "  have been applied to the templates"
echo ""
while true; do
	read -p "Generate source files? " yn
	case $yn in
		[Yy]* ) break;;
		[Nn]* ) exit;;
		* ) echo "Please answer y or n";;
	esac
done

# using mustach from https://gitlab.com/jobol/mustach

# 8 bit

mustach templates/8bit/mustache_1d.json templates/phtree_header_template.h > source/8bit/phtree8_1d.h
mustach templates/8bit/mustache_1d.json templates/phtree_source_template.c > source/8bit/phtree8_1d.c

mustach templates/8bit/mustache_2d.json templates/phtree_header_template.h > source/8bit/phtree8_2d.h
mustach templates/8bit/mustache_2d.json templates/phtree_source_template.c > source/8bit/phtree8_2d.c

mustach templates/8bit/mustache_3d.json templates/phtree_header_template.h > source/8bit/phtree8_3d.h
mustach templates/8bit/mustache_3d.json templates/phtree_source_template.c > source/8bit/phtree8_3d.c

mustach templates/8bit/mustache_4d.json templates/phtree_header_template.h > source/8bit/phtree8_4d.h
mustach templates/8bit/mustache_4d.json templates/phtree_source_template.c > source/8bit/phtree8_4d.c

mustach templates/8bit/mustache_5d.json templates/phtree_header_template.h > source/8bit/phtree8_5d.h
mustach templates/8bit/mustache_5d.json templates/phtree_source_template.c > source/8bit/phtree8_5d.c

mustach templates/8bit/mustache_6d.json templates/phtree_header_template.h > source/8bit/phtree8_6d.h
mustach templates/8bit/mustache_6d.json templates/phtree_source_template.c > source/8bit/phtree8_6d.c


# 16 bit

mustach templates/16bit/mustache_1d.json templates/phtree_header_template.h > source/16bit/phtree16_1d.h
mustach templates/16bit/mustache_1d.json templates/phtree_source_template.c > source/16bit/phtree16_1d.c

mustach templates/16bit/mustache_2d.json templates/phtree_header_template.h > source/16bit/phtree16_2d.h
mustach templates/16bit/mustache_2d.json templates/phtree_source_template.c > source/16bit/phtree16_2d.c

mustach templates/16bit/mustache_3d.json templates/phtree_header_template.h > source/16bit/phtree16_3d.h
mustach templates/16bit/mustache_3d.json templates/phtree_source_template.c > source/16bit/phtree16_3d.c

mustach templates/16bit/mustache_4d.json templates/phtree_header_template.h > source/16bit/phtree16_4d.h
mustach templates/16bit/mustache_4d.json templates/phtree_source_template.c > source/16bit/phtree16_4d.c

mustach templates/16bit/mustache_5d.json templates/phtree_header_template.h > source/16bit/phtree16_5d.h
mustach templates/16bit/mustache_5d.json templates/phtree_source_template.c > source/16bit/phtree16_5d.c

mustach templates/16bit/mustache_6d.json templates/phtree_header_template.h > source/16bit/phtree16_6d.h
mustach templates/16bit/mustache_6d.json templates/phtree_source_template.c > source/16bit/phtree16_6d.c


# 32 bit

mustach templates/32bit/mustache_1d.json templates/phtree_header_template.h > source/32bit/phtree32_1d.h
mustach templates/32bit/mustache_1d.json templates/phtree_source_template.c > source/32bit/phtree32_1d.c

mustach templates/32bit/mustache_2d.json templates/phtree_header_template.h > source/32bit/phtree32_2d.h
mustach templates/32bit/mustache_2d.json templates/phtree_source_template.c > source/32bit/phtree32_2d.c

mustach templates/32bit/mustache_3d.json templates/phtree_header_template.h > source/32bit/phtree32_3d.h
mustach templates/32bit/mustache_3d.json templates/phtree_source_template.c > source/32bit/phtree32_3d.c

mustach templates/32bit/mustache_4d.json templates/phtree_header_template.h > source/32bit/phtree32_4d.h
mustach templates/32bit/mustache_4d.json templates/phtree_source_template.c > source/32bit/phtree32_4d.c

mustach templates/32bit/mustache_5d.json templates/phtree_header_template.h > source/32bit/phtree32_5d.h
mustach templates/32bit/mustache_5d.json templates/phtree_source_template.c > source/32bit/phtree32_5d.c

mustach templates/32bit/mustache_6d.json templates/phtree_header_template.h > source/32bit/phtree32_6d.h
mustach templates/32bit/mustache_6d.json templates/phtree_source_template.c > source/32bit/phtree32_6d.c


# 64 bit

mustach templates/64bit/mustache_1d.json templates/phtree_header_template.h > source/64bit/phtree64_1d.h
mustach templates/64bit/mustache_1d.json templates/phtree_source_template.c > source/64bit/phtree64_1d.c

mustach templates/64bit/mustache_2d.json templates/phtree_header_template.h > source/64bit/phtree64_2d.h
mustach templates/64bit/mustache_2d.json templates/phtree_source_template.c > source/64bit/phtree64_2d.c

mustach templates/64bit/mustache_3d.json templates/phtree_header_template.h > source/64bit/phtree64_3d.h
mustach templates/64bit/mustache_3d.json templates/phtree_source_template.c > source/64bit/phtree64_3d.c

mustach templates/64bit/mustache_4d.json templates/phtree_header_template.h > source/64bit/phtree64_4d.h
mustach templates/64bit/mustache_4d.json templates/phtree_source_template.c > source/64bit/phtree64_4d.c

mustach templates/64bit/mustache_5d.json templates/phtree_header_template.h > source/64bit/phtree64_5d.h
mustach templates/64bit/mustache_5d.json templates/phtree_source_template.c > source/64bit/phtree64_5d.c

mustach templates/64bit/mustache_6d.json templates/phtree_header_template.h > source/64bit/phtree64_6d.h
mustach templates/64bit/mustache_6d.json templates/phtree_source_template.c > source/64bit/phtree64_6d.c
