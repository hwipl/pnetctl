project('pnetctl', 'c')
cc = meson.get_compiler('c')
pnetctldep = dependency('udev')
lib_udev = cc.find_library('udev')
executable('pnetctl',
           'pnetctl.c',
           dependencies : [pnetctldep, lib_udev],
           install : true)
