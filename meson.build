project('helium', 'cpp',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'default_library=static',
    'buildtype=release',
  ])

if not meson.is_subproject()
  add_global_arguments('-Wno-gnu-case-range', language: 'cpp')
  add_global_arguments('-Wno-gnu-statement-expression', language: 'cpp')
  add_global_arguments('-Wno-gnu-conditional-omitted-operand', language: 'cpp')
endif

core_proj = subproject('Core')
core_dep = core_proj.get_variable('core_dep')
cli_dep = core_proj.get_variable('cli_dep')
types_dep = subproject('Types').get_variable('types_dep')

helium_exe = executable('helium', [
    'src/main.cpp',
    'src/He/Codegen.cpp',
    'src/He/Expression.cpp',
    'src/He/Lexer.cpp',
    'src/He/Parser.cpp',
    'src/He/Token.cpp',
    'src/He/Typecheck.cpp',
  ],
  include_directories: 'src',
  dependencies: [
    core_dep,
    cli_dep,
    types_dep,
  ])
