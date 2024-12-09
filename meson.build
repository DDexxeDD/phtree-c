project('phtree', 'c')

cc = meson.get_compiler('c')

include = [
  include_directories('.'),
  include_directories('external/cvector'),
]

source_files = [
  'source/main.c',
  'source/phtree.c',
  'source/print.c',
]

phtree_binary = executable (
  'phtree',
  source_files,
  include_directories : include,
)
