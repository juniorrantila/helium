
selfhost_exe = executable('helium', [
    bootstrap_gen.process('Lexer.he'),
    bootstrap_gen.process('Token.he'),
    bootstrap_gen.process('main.he'),
  ],
  c_args: [
    '-std=gnu2x',
    '-Wno-gnu-auto-type',
    '-Wno-fixed-enum-extension',
    '-Wno-gnu-empty-initializer',
    '-Wno-dollar-in-identifier-extension',
    '-Wno-duplicate-decl-specifier',
    '-Wno-language-extension-token'
  ],
  include_directories: '.',
  dependencies: [
    core_dep,
    types_dep,
  ])
