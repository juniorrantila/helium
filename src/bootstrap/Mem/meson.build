mem_lib = library('mem', [
    'AddressSpace.cpp',
  ],
  dependencies: [
    core_dep,
    ty_dep,
  ])

mem_dep = declare_dependency(
  link_with: mem_lib,
  include_directories: '..'
  )

