core_lib = library('core', [
    'DenseMap.cpp',
    'Error.cpp',
    'File.cpp',
    'GenericVector.cpp',
    'MappedFile.cpp',
    'System.cpp',
    ],
    dependencies: types_dep)

core_dep = declare_dependency(
  link_with: core_lib,
  include_directories: '..'
  )
