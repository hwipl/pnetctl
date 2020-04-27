project('pnetctl', 'c')
cc = meson.get_compiler('c')
pnetctldep = [dependency('libudev'),
              dependency('libnl-3.0'),
              dependency('libnl-genl-3.0')]
executable('pnetctl',
           'src/pnetctl.c',
           dependencies : pnetctldep,
           install : true)
