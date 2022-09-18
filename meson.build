project('helium', ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'c_std=c2x',
    'warning_level=3',
    'default_library=static',
    'buildtype=release',
  ])

if not meson.is_subproject()
  add_global_arguments([
      '-Wno-c99-extensions',
      '-Wno-dollar-in-identifier-extension', 
      '-Wno-gnu-auto-type',
      '-Wno-gnu-case-range', 
      '-Wno-gnu-conditional-omitted-operand', 
      '-Wno-gnu-statement-expression', 
    ], language: ['c', 'cpp'])
endif

core_proj = subproject('Core')
core_dep = core_proj.get_variable('core_dep')
cli_dep = core_proj.get_variable('cli_dep')
types_dep = subproject('Types').get_variable('types_dep')

stage_0_exe = executable('stage-0', [
    'src/bootstrap/main.cpp',
    'src/bootstrap/He/TypecheckedExpression.cpp',
    'src/bootstrap/He/Expression.cpp',
    'src/bootstrap/He/Lexer.cpp',
    'src/bootstrap/He/Parser.cpp',
    'src/bootstrap/He/Token.cpp',
    'src/bootstrap/He/Typecheck.cpp',
  ],
  include_directories: 'src/bootstrap',
  dependencies: [
    core_dep,
    cli_dep,
    types_dep,
  ])

stage_0_c_gen = generator(stage_0_exe,
  output: '@BASENAME@.c',
  arguments: ['@INPUT@', '-S', '-o', '@OUTPUT@'],
  depends: stage_0_exe,
  )

stage_1_exe = executable('stage-1', [
    stage_0_c_gen.process('src/selfhost/Lexer.he'),
    stage_0_c_gen.process('src/selfhost/Token.he'),
    stage_0_c_gen.process('src/selfhost/main.he'),
  ],
  c_args: [
    '-std=c2x',
    '-Wno-gnu-auto-type',
    '-Wno-fixed-enum-extension',
    '-Wno-gnu-empty-initializer',
    '-Wno-dollar-in-identifier-extension',
    '-Wno-duplicate-decl-specifier',
  ],
  include_directories: 'src/selfhost',
  dependencies: [
    core_dep,
    types_dep,
    ])

subdir('samples')
