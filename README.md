# pnetctl

pnetctl is a Linux command line tool for configuring and listing physical
network IDs (pnetids) of devices. The [SMC
protocol](https://www.rfc-editor.org/info/rfc7609) implementation in Linux uses
pnetids to determine which network and infiniband devices are attached to the
same physical network and, thus, can be used together for SMC connections.

## Installation

In order to compile, install, and use pnetctl, you need to have it's
dependencies available on your system:

* libudev
* libnl
* libnl-genl

You can compile and install pnetctl with the meson build system using the
following commands:

* `meson builddir`
* `ninja -C builddir install`


## Usage

You can run pnetctl with and without command line arguments:

```
------------------------------------------------------------
Usage:
------------------------------------------------------------
pnetctl                 Print all devices and pnetids
pnetctl <options>       Run commands specified by options
                        (see below)
------------------------------------------------------------
Options:
------------------------------------------------------------
-a <pnetid>             Add pnetid. Requires -n or -i
-r <pnetid>             Remove pnetid
-f                      Flush pnetids
-n <net_dev>            Specify net device
-i <ib_dev>             Specify infiniband device
-p <ib_port>            Specify infiniband port
                        (default: 1)
-h                      Print this help
```

Running pnetctl without command line argument prints all devices and their
pnetids found on your system. Command line arguments can be used to add,
remove, and flush pnetids.


## Output

If you run pnetctl without command line arguments, it prints out the list of
devices and pnetids found on your system. For example, the output could look
like this:

```
==============================================================
Pnetid:          Type:           Name:  Port:          PCI-ID:
==============================================================
TESTID
--------------------------------------------------------------
                   net            eth1            0000:02:00.0
                    ib          mlx5_0      1     0000:03:00.0
--------------------------------------------------------------
n/a
--------------------------------------------------------------
                   net            eth0            0000:01:00.0
                   net              lo                     n/a
```

The *pnetid* column shows the pnetids found on your system. For each pnetid,
you can see the devices with their *type*, *name*, *port* (for ib devices), and
*PCI-ID* in the respective columns. The special pnetid *n/a* is used to display
all devices without a pnetid. Supported device types are linux net devices
(*net*) like, e.g., `lo` or `eth0` and infiniband devices (*ib*) like, e.g.,
`mlx4_1` or `mlx5_0`.
