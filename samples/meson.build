
samples_c_args = [
    '-std=c2x',
    '-Wno-gnu-auto-type',
    '-Wno-fixed-enum-extension',
    '-Wno-gnu-empty-initializer',
    '-Wno-dollar-in-identifier-extension',
    '-Wno-duplicate-decl-specifier',
    '-Wno-extra-semi',
  ]

executable('fib', stage_0_c_gen.process('fib.he'), c_args: samples_c_args)
executable('global', stage_0_c_gen.process('global.he'), c_args: samples_c_args)
executable('hello-world', stage_0_c_gen.process('hello-world.he'), c_args: samples_c_args)
executable('inline-c', stage_0_c_gen.process('inline-c.he'), c_args: samples_c_args)
executable('struct', stage_0_c_gen.process('struct.he'), c_args: samples_c_args)

if build_machine.system() == 'linux'
executable('watchf', stage_0_c_gen.process('watchf.he'), c_args: [
    samples_c_args,
    '-D_POSIX_C_SOURCE'
  ])
endif
