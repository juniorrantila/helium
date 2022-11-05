subdir('Ty')
subdir('Core')
subdir('Mem')
subdir('CLI')
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
  output: ['@PLAINNAME@.c', '@PLAINNAME@.h'],
  arguments: ['@INPUT@', '-S', '-o', '@OUTPUT0@', '-oh', '@OUTPUT1@'],
  depends: bootstrap_exe,
  )
