project('phtree', 'c')

cc = meson.get_compiler('c')

include = [
  include_directories('.'),
  include_directories('examples/external/cvector'),
]

phtree_dependencies = [
  cc.find_library('m'),
]

demo_1d_files = [
  #'source/32bit/phtree32_common.c',
  'source/32bit/phtree32_1d.c',
  'examples/demo_1d.c',
]

demo_1d_binary = executable (
  'demo_1d',
  demo_1d_files,
  include_directories : include,
  dependencies : [phtree_dependencies],
)

demo_2d_spatial_files = [
  #'source/32bit/phtree32_common.c',
  'source/32bit/phtree32_2d.c',
  'examples/demo_2d_spatial_hash.c',
]

demo_2d_spatial_binary = executable (
  'demo_2d_spatial',
  demo_2d_spatial_files,
  include_directories : include,
  dependencies : [phtree_dependencies],
)

demo_2d_simple_files = [
  #'source/32bit/phtree32_common.c',
  'source/32bit/phtree32_2d.c',
  'examples/demo_2d_simple.c',
]

demo_2d_simple_binary = executable (
  'demo_2d_simple',
  demo_2d_simple_files,
  include_directories : include,
  dependencies : [phtree_dependencies],
)

demo_3d6d_files = [
  #'source/32bit/phtree32_common.c',
  'source/32bit/phtree32_3d.c',
  'source/32bit/phtree32_6d.c',
  'examples/demo_3d6d.c',
]

demo_3d6d_binary = executable (
  'demo_3d6d',
  demo_3d6d_files,
  include_directories : include,
  dependencies : [phtree_dependencies],
)
