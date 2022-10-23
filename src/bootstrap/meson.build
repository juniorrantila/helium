subdir('Ty')
subdir('Core')
subdir('CLI')
subdir('Mem')
subdir('He')
subdir('Main')

bootstrap_exe = executable('helium', [
    'main.cpp',
  ],
  include_directories: '.',
  dependencies: [
    cli_dep,
    core_dep,
    he_dep,
    main_dep,
    mem_dep,
    ty_dep,
  ])

bootstrap_gen = generator(bootstrap_exe,
  output: '@BASENAME@.c',
  arguments: ['@INPUT@', '-S', '-o', '@OUTPUT@'],
  depends: bootstrap_exe,
  )
