wimboot: Windows Imaging Format bootloader
==========================================

[![Build](https://img.shields.io/github/workflow/status/ipxe/wimboot/Build)](https://github.com/ipxe/wimboot/actions?query=workflow%3ABuild+branch%3Amaster)
[![Coverity](https://img.shields.io/coverity/scan/12613)](https://scan.coverity.com/projects/ipxe-wimboot)
[![Tests](https://img.shields.io/github/workflow/status/ipxe/wimboot/QA?label=tests)](https://github.com/ipxe/wimboot/actions?query=workflow%3AQA+branch%3Amaster)
[![Release](https://img.shields.io/github/v/release/ipxe/wimboot)](https://github.com/ipxe/wimboot/releases/latest)

[`wimboot`][wimboot] is a boot loader for Windows Imaging Format
`.wim` files.  It enables you to boot into a [Windows PE
(WinPE)][winpe] deployment or recovery environment.

You can use `wimboot` with [iPXE][ipxe] to [boot Windows PE via
HTTP][howto].  With a Gigabit Ethernet network, a typical WinPE image
should download in just a few seconds.

Demo
----

This animation shows an HTTP network boot into the Windows 10
installer.  The total elapsed time from power on to reaching the
Windows installer screen is 17 seconds.

![Demo animation](doc/demo.gif)

Advantages
----------

### Speed

`wimboot` can download images at the full speed supported by your
network, since it can use HTTP rather than TFTP.

### Ease of use

`wimboot` works directly with `.wim` image files; there is no need to
wrap your `.wim` into an ISO or FAT filesystem image.

### BIOS/UEFI compatibility

`wimboot` allows you to use a single configuration and set of files to
boot under both BIOS and UEFI environments.

License
-------

`wimboot` is free software licensed under the [GNU General Public
License](LICENSE.txt).

Quickstart
----------

See https://ipxe.org/wimboot for instructions.


[wimboot]: https://ipxe.org/wimboot
[ipxe]: https://ipxe.org
[winpe]: https://en.wikipedia.org/wiki/Windows_Preinstallation_Environment
[howto]: https://ipxe.org/howto/winpe
