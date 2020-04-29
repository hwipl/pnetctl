project('pnetctl', 'c')
cc = meson.get_compiler('c')
pnetctldep = [dependency('libudev'),
              dependency('libnl-3.0'),
              dependency('libnl-genl-3.0')]
pnetctlsrc = ['src/pnetctl.c', 'src/netlink.c', 'src/devices.c', 'src/udev.c']
exe = executable('pnetctl',
           sources : ['src/main.c'] + pnetctlsrc,
           dependencies : pnetctldep,
           install : true)

# parallel cli argument tests
test('help',
  exe,
  args : ['-h'],
  suite : 'cli')
test('help verbose',
  exe,
  args : ['-v', '-h'],
  suite : 'cli')

# sequential cli tests (no infiniband)
test('add',
  exe,
  args : ['-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false,
  priority : -100,
  suite : 'cli')
test('get all',
  exe,
  is_parallel : false,
  priority : -110,
  suite : 'cli')
test('get',
  exe,
  args : ['-g', 'PNETCTL'],
  is_parallel : false,
  priority : -120,
  suite : 'cli')
test('remove',
  exe,
  args : ['-r', 'PNETCTL'],
  is_parallel : false,
  priority : -130,
  suite : 'cli')
test('get all',
  exe,
  is_parallel : false,
  priority : -140,
  suite : 'cli')
test('add',
  exe,
  args : ['-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false,
  priority : -150,
  suite : 'cli')
test('flush',
  exe,
  args : ['-f'],
  is_parallel : false,
  priority : -160,
  suite : 'cli')
test('get all',
  exe,
  is_parallel : false,
  priority : -170,
  suite : 'cli')

# sequential cli tests, verbose (no infiniband)
test('add verbose',
  exe,
  args : ['-v', '-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false,
  priority : -200,
  suite : 'cli')
test('get all verbose',
  exe,
  args : ['-v'],
  is_parallel : false,
  priority : -210,
  suite : 'cli')
test('get verbose',
  exe,
  args : ['-v', '-g', 'PNETCTL'],
  is_parallel : false,
  priority : -220,
  suite : 'cli')
test('remove verbose',
  exe,
  args : ['-v', '-r', 'PNETCTL'],
  is_parallel : false,
  priority : -230,
  suite : 'cli')
test('get all verbose',
  exe,
  args : ['-v'],
  is_parallel : false,
  priority : -240,
  suite : 'cli')
test('add verbose',
  exe,
  args : ['-v', '-a', 'PNETCTL', '-n', 'lo'],
  is_parallel : false,
  priority : -250,
  suite : 'cli')
test('flush verbose',
  exe,
  args : ['-v', '-f'],
  is_parallel : false,
  priority : -260,
  suite : 'cli')
test('get all verbose',
  exe,
  args : ['-v'],
  is_parallel : false,
  priority : -270,
  suite : 'cli')

# devices tests
devices_test_exe = executable('devices_test',
  sources : ['src/devices_test.c'] + pnetctlsrc,
  dependencies : pnetctldep)

test('new_device',
  devices_test_exe,
  args : ['new_device'],
  suite : 'devices')
test('get_next_device',
  devices_test_exe,
  args : ['get_next_device'],
  suite : 'devices')
test('set_pnetid_for_eth',
  devices_test_exe,
  args : ['set_pnetid_for_eth'],
  suite : 'devices')
test('set_pnetid_for_ib',
  devices_test_exe,
  args : ['set_pnetid_for_ib'],
  suite : 'devices')
test('free_devices',
  devices_test_exe,
  args : ['free_devices'],
  suite : 'devices')

# netlink tests
netlink_test_exe = executable('netlink_test',
  sources : ['src/netlink_test.c'] + pnetctlsrc,
  dependencies : pnetctldep)

test('nl_init',
  netlink_test_exe,
  args : ['nl_init'],
  suite : 'netlink')
test('nl_cleanup',
  netlink_test_exe,
  args : ['nl_cleanup'],
  suite : 'netlink')
test('nl_flush_pnetids',
  netlink_test_exe,
  args : ['nl_flush_pnetids'],
  suite : 'netlink')
test('nl_del_pnetid',
  netlink_test_exe,
  args : ['nl_del_pnetid'],
  suite : 'netlink')
test('nl_set_pnetid',
  netlink_test_exe,
  args : ['nl_set_pnetid'],
  suite : 'netlink')
test('nl_get_pnetids',
  netlink_test_exe,
  args : ['nl_get_pnetids'],
  suite : 'netlink')

# udev tests
udev_test_exe = executable('udev_test',
  sources : ['src/udev_test.c'] + pnetctlsrc,
  dependencies : pnetctldep)

test('udev_scan_devices',
  udev_test_exe,
  args : ['udev_scan_devices'],
  suite : 'udev')
