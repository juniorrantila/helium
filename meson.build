project('helium', ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'c_std=gnu2x',
    'warning_level=3',
    'default_library=static',
    'buildtype=release',
    'cpp_rtti=false',
  ])

if not meson.is_subproject()
  add_global_arguments([
      '-Wno-c99-extensions',
      '-Wno-dollar-in-identifier-extension', 
      '-Wno-gnu-auto-type',
      '-Wno-gnu-case-range', 
      '-Wno-gnu-conditional-omitted-operand', 
      '-Wno-gnu-statement-expression', 
      '-Wno-user-defined-literals',
    ], language: ['c', 'cpp'])
endif

subdir('src')
subdir('samples')
