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

## Usage Examples

See examples in directory `examples`.

## Installation

Currently, the driver has only been tried on the BeagleBone Black
running a 3.8 kernel.  Attempts to compile on 4.15 fail, as the
interface to kfifo seems to be slightly different; have not
investigated this issue.

Currently, installation is manual and error prone:

1. Create a group muons and add users that need this device to that
group 
1. Copy the udev rules file to the appropriate place in /etc
1. Install kernel module build tools.
1. Run 'make' in the BBB_Kernel_Driver subdirectory to build the
kernel module
1. Manually load as root via 'insmod muon_timer.ko'.  Check the code
comments for load time options (debug, pin remappings, etc); this part
can, of course, be automated at boot time depending on your
distribution. 

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

## License

GPL v2 or later

## References

The following provided much needed encouragement and information about the ins and outs of writing this driver; mistakes and poor application of idiom are all ours:

* [Derek Molloy's Blog](http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)
* [Linux Device Drivers, 3rd Edition](http://free-electrons.com/doc/books/ldd3.pdf)
  * [Linux Device Drivers, 3rd Edition Example Code](http://examples.oreilly.com/linuxdrive3)
* [Pete's Blog](http://pete.akeo.ie/2011/08/writing-linux-device-driver-for-kernels.html)

