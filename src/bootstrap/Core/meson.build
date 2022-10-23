core_lib = library('core', [
    'Error.cpp',
    'File.cpp',
    'MappedFile.cpp',
    'System.cpp',
    ],
    dependencies: ty_dep)

core_dep = declare_dependency(
  link_with: core_lib,
  include_directories: '..'
  )
