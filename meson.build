project('pnetctl', 'c')
cc = meson.get_compiler('c')
pnetctldep = [dependency('libudev'),
              dependency('libnl-3.0'),
              dependency('libnl-genl-3.0')]
pnetctlsrc = ['src/pnetctl.c', 'src/netlink.c', 'src/devices.c']
executable('pnetctl',
           sources : pnetctlsrc,
           dependencies : pnetctldep,
           install : true)
