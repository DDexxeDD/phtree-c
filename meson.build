project('phtree', 'c')

include = [
  include_directories('.'),
  include_directories('example/external/cvector'),
]

source_files = [
  'source/phtree32_2d.c',
  'example/main.c',
]

phtree_binary = executable (
  'phtree',
  source_files,
  include_directories : include,
)
