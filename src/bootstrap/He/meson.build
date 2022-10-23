
he_lib = library('he', [
    'Codegen.cpp',
    'Expression.cpp',
    'Lexer.cpp',
    'Parser.cpp',
    'Token.cpp',
    'Typecheck.cpp',
  ],
  dependencies: [
    core_dep,
    cli_dep,
    mem_dep,
    ty_dep,
  ])

he_dep = declare_dependency(
  link_with: he_lib,
  include_directories: '..'
  )
