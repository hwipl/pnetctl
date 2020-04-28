project('pnetctl', 'c')
cc = meson.get_compiler('c')
pnetctldep = [dependency('libudev'),
              dependency('libnl-3.0'),
              dependency('libnl-genl-3.0')]
pnetctlsrc = ['src/main.c','src/pnetctl.c', 'src/netlink.c', 'src/devices.c',
              'src/udev.c']
exe = executable('pnetctl',
           sources : pnetctlsrc,
           dependencies : pnetctldep,
           install : true)

# parallel cli argument tests
test('cli: help', exe, args : ['-h'])
test('cli: help verbose', exe, args : ['-v', '-h'])

# sequential cli tests (no infiniband)
test('cli: add', exe, args : ['-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false, priority : -100)
test('cli: get all', exe,
  is_parallel : false, priority : -110)
test('cli: get', exe, args : ['-g', 'PNETCTL'],
  is_parallel : false, priority : -120)
test('cli: remove', exe, args : ['-r', 'PNETCTL'],
  is_parallel : false, priority : -130)
test('cli: get all', exe,
  is_parallel : false, priority : -140)
test('cli: add', exe, args : ['-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false, priority : -150)
test('cli: flush', exe, args : ['-f'],
  is_parallel : false, priority : -160)
test('cli: get all', exe,
  is_parallel : false, priority : -170)

# sequential cli tests, verbose (no infiniband)
test('cli: add verbose', exe, args : ['-v', '-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false, priority : -200)
test('cli: get all verbose', exe, args : ['-v'],
  is_parallel : false, priority : -210)
test('cli: get verbose', exe, args : ['-v', '-g', 'PNETCTL'],
  is_parallel : false, priority : -220)
test('cli: remove verbose', exe, args : ['-v', '-r', 'PNETCTL'],
  is_parallel : false, priority : -230)
test('cli: get all verbose', exe, args : ['-v'],
  is_parallel : false, priority : -240)
test('cli: add verbose', exe, args : ['-v', '-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false, priority : -250)
test('cli: flush verbose', exe, args : ['-v', '-f'],
  is_parallel : false, priority : -260)
test('cli: get all verbose', exe, args : ['-v'],
  is_parallel : false, priority : -270)
