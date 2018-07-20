# muon_timer
SBC Drivers and code for cosmic ray muon timing

## Synopsis

This repo provides code to implement Linux kernel drivers for the
BeagleBone Black to do timing studies for cosmic ray muon tomography
using GPIO interrupts.

## Motivation

Our experimental particle physics research group at [York
College/CUNY](http://www.york.cuny.edu) is developing a robust,
portable, modular cosmic ray muon tomography system with research
funding from the Air Force Office of Scientific Research.  Our goal is
to enable detection of hidden and shielded nuclear materials using -
to the extent possible - robust, off-the-shelf computing devices and
inexpensive plastic scintillator paddles.  The core of each detector
module is a single board computer (SBC), such as the well-known
Raspberry Pi or BeagleBone Black, slaved to a backend data collection
and control PC.

## Capabilities

The driver utilizes the SBC as an updating TDC.  Asynchronously
arriving physics events drive an external discriminator circuit onto a
latch that provides the input signal to and SBC GPIO pin.  The input
signal raises an interrupt; the handler provides a time stamping
service (of c-type `struct timespec`) and schedules a reset of the
latch via an additional GPIO pin.  Access to a second output pin is
provided by the driver, primarily for testing and debugging purposes.
Control of the pins is provided through sysfs.  Readout of the
timestamp data stream is provided through /dev/muon_timer, which
provides a FIFO of binary `struct timespec` objects.  Only one process
may be accessing the device at any time.

This code package also contains a small HTTP server, which serves real-
time data from /dev/muon_timer in response to a GET request.

## Usage Examples

See examples in directory `examples`.

## Installation

Currently, the driver has been tried on the BeagleBone Black running
a 3.8 kernel, and the Raspberry Pi on a 4.4 kernel.

To install on either of these systems, run `install.sh`.

If you would rather install muon_timer manually, for instance on a 
different system or with debug logging switched on, follow these 
steps:

1. Create a group muons and add users that need this device to that
group 
1. Copy the udev rules file to the appropriate place in /etc
1. Install kernel module build tools.
1. Run 'make' in either the RPI_Kernel_Driver subdirectory (for kernel 
4.4) or the BBB_Kernel_Driver subdirectory (for kernel 3.8) to build 
the kernel module
1. To test the driver, manually load as root via 'insmod
muon_timer.ko'.  Check the code comments for load time options (debug,
pin remappings, etc); this part can, of course, be automated at boot
time depending on your distribution.
1. To start the event server, run `./server_start.sh start` in the 
coincidences subdirectory. By default, the server listens on port 
8090 .

To automate the module loading at boot time (on debian wheezy at
least), do the following

1. As root, create a directory `/lib/modules/$(uname -r)/muon_timer`.
1. Copy the module ko file to that directory.
1. Run `depmod` as root to recreate the module dependency tree.
1. Copy the contents of `modules-load.d` to `/etc/modules-load.d`.
1. Reboot and check that the driver has been loaded with `lsmod`.

Eventually, we will have (full) udev support and (hopefully) better
installation support, since we need that ourselves anyway.  At this
time, because of the lack of udev support for user and group ownership
of sysfs nodes, permissions are looser than one might strictly prefer.
But, as long as it 'works' well enough for our project, installation
will likely never be terribly polished.

## Tests and examples

There are a number of test and example programs in the `tests` and
`examples` directories.  If the driver is loaded with the 'debug=1'
option, significant additional information on the operation of the
driver will be output to the kernel log.

## Contributors

* [Kevin Lynch](mailto:klynch@york.cuny.edu)
* Luis Maduro
* Helenka Casler

## License

GPL v2 or later

## References

The following provided much needed encouragement and information about the ins and outs of writing this driver; mistakes and poor application of idiom are all ours:

* [Derek Molloy's Blog](http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)
* [Linux Device Drivers, 3rd Edition](http://free-electrons.com/doc/books/ldd3.pdf)
  * [Linux Device Drivers, 3rd Edition Example Code](http://examples.oreilly.com/linuxdrive3)
* [Pete's Blog](http://pete.akeo.ie/2011/08/writing-linux-device-driver-for-kernels.html)

