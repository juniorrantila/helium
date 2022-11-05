
samples_c_args = [
    '-std=c2x',
    '-Wno-gnu-auto-type',
    '-Wno-fixed-enum-extension',
    '-Wno-gnu-empty-initializer',
    '-Wno-dollar-in-identifier-extension',
    '-Wno-duplicate-decl-specifier',
    '-Wno-extra-semi',
    '-Wno-pedantic',
  ]

executable('enum', bootstrap_gen.process('enum.he'), c_args: samples_c_args)
executable('fib', bootstrap_gen.process('fib.he'), c_args: samples_c_args)
executable('global', bootstrap_gen.process('global.he'), c_args: samples_c_args)
executable('hello-world', bootstrap_gen.process('hello-world.he'), c_args: samples_c_args)
executable('inline-c', bootstrap_gen.process('inline-c.he'), c_args: samples_c_args)
executable('struct', bootstrap_gen.process('struct.he'), c_args: samples_c_args)
executable('union', bootstrap_gen.process('union.he'), c_args: samples_c_args)
executable('variant', bootstrap_gen.process('variant.he'), c_args: samples_c_args)

if build_machine.system() == 'linux'
executable('watchf', bootstrap_gen.process('watchf.he'), c_args: [
    samples_c_args,
    '-D_POSIX_C_SOURCE'
  ])
endif

subdir('multi-source')
