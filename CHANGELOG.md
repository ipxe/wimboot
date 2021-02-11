Changelog
=========

## [Unreleased]

## [v2.7.0] 2021-02-11

- Extract `BCD`, `boot.sdi`, and standard boot font files
  automatically from the `.wim` image.

- Use paging and Physical Address Extensions (PAE) to place the initrd
  above 4GB if possible, thereby allowing larger `.wim` images to be
  used on BIOS systems (sponsored by [Digital Intelligence][digint]).

- Enable stack protection for both BIOS and UEFI builds.

- Tidy up debug output that typically disrupts the loading screen
  progress bar.

- Add `quiet` command-line option to inhibit all debug output
  (sponsored by [2Pint Software][2pint]).

- Create fully automated tests of an HTTP boot via iPXE and wimboot
  into multiple versions of Windows.

- Migrate from [Travis CI][travis] to [GitHub actions][actions].

## [v2.6.0] 2017-05-10

- Dynamically patch the `.wim` image to allow files to be injected in
  `\Windows\System32`, allowing for fine-grained control of the boot
  process (sponsored by [2Pint Software][2pint]).

- Use [Coverity Scan][coverity] for ongoing static analysis.

## [v2.5.2] 2016-02-09

- Validate signed binaries to ensure that no malicious code was
  injected in the UEFI Secure Boot signing process.

- Fix identification of terminating character in strtoul().

## [v2.5.1] 2015-09-22

- Work around broken 32-bit PE executable parsing in the UEFI Secure
  Boot signing process.

- Provide UEFI Secure Boot signed binaries.

## [v2.5.0] 2015-08-28

- Limit initrd to the low 2GB of memory to work around versions of
  Windows that assume the ability to use arbitrary fixed addresses
  above 0x80000000.

- Prepare for UEFI Secure Boot signing.

## [v2.4.1] 2014-12-08

- Ignore `initrdfile` command-line argument that may be appended by
  syslinux.

## [v2.4.0] 2014-11-07

- Provide space for a protected-mode interrupt descriptor table,
  avoiding RSOD crashes on systems such as HP DL360p Gen8 that happen
  to occasionally generate IRQ 5 during boot.

## [v2.3.0] 2014-09-24

- Fix building of 32-bit UEFI binary.

## [v2.2.5] 2014-09-23

- Fix construction of FAT long filenames that are exactly 12
  characters in length.

## [v2.2.4] 2014-09-09

- Work around UEFI firmware that fails to populate the `DeviceHandle`
  in `EFI_LOADED_IMAGE_PROTOCOL`.

## [v2.2.3] 2014-09-03

- Display an optional prompt when using the `pause` command-line
  option.

## [v2.2.2] 2014-09-03

- Generate position-independent code for UEFI, to allow for relocation
  to arbitrary 64-bit addresses.

- Fix assorted build toolchain issues.

## [v2.2.1] 2014-09-02

- Respect the selected boot index when extracting files from the
  `.wim` image.

- Add `pause` command-line option to allow for inspection of debug
  output before transferring control to `bootmgr.exe` or
  `bootmgfw.efi`.

## [v2.2.0] 2014-09-02

- Add the ability to extract individual files from the `.wim` image.

- Extract `bootmgr.exe` or `bootmgfw.efi` automatically from the
  `.wim` image.

## [v2.1.0] 2014-08-30

- Dynamically patch `BCD` file to allow the same file to be used for
  both BIOS and UEFI boot.

- Force `bootmgfw.efi` to use text mode (as already done for
  `bootmgr.exe`).

- Add `rawbcd` command-line option to disable dynamic BCD patching.

- Add `gui` command-line option to disable forced text mode output.

- Add `index=N` command-line option to allow a boot image to be
  selected dynamically at runtime.

## [v2.0.0] 2014-08-29

- Support booting on UEFI systems (sponsored by [Jump Trading][jump]).

- Retrieve initrd files via `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL`.

- Expose the virtual FAT filesystem to `bootmgfw.efi` via
  `EFI_BLOCK_IO_PROTOCOL`.

- Build as a hybrid 32-bit BIOS and 64-bit UEFI binary.

## [v1.0.6] 2014-06-22

- Make real-mode setup code relocatable.

## [v1.0.5] 2014-05-17

- Force `bootmgr.exe` to use text mode since it is unable to display
  error messages in graphical mode (unless the appropriate set of font
  files has been added).

## [v1.0.4] 2014-03-19

- Fix the XCA decompressor to match the undocumented behaviour of the
  compressor when crossing 64kB block boundaries.

## [v1.0.3] 2012-11-11

- Accept arbitrary amounts of zero-padding between initrds.

## [v1.0.2] 2012-10-30

- Accept initrds that are not padded to an alignment boundary.

## [v1.0.1] 2012-10-24

- Accept initrds with CPIO trailers to allow booting via syslinux as
  well as iPXE.

## [v1.0.0] 2012-09-17

- Support XCA compression of `bootmgr.exe` as used by Windows 8 and
  Windows Server 2012.

## [v0.9.3] 2012-09-15

- Expose files via `\Boot\Resources` directory for Windows 8.

## [v0.9.2] 2012-09-14

- Select virtual disk drive number dynamically to allow Windows to
  identify existing hard disks as being bootable.

## [v0.9.1] 2012-09-13

- Include pre-built binaries to make life easier for Windows users.

## [v0.9] 2012-09-11

- Create first working implementation (sponsored by [Jump
  Trading][jump]).

- Provide a BOOTAPP entry point usable by `bootmgr.exe`.

- Extract embedded `bootmgr.exe` from within `bootmgr`, if provided.

- Provide a virtual BIOS disk drive containing a read-only FAT
  filesystem to expose files to `bootmgr.exe`.

- Expose files via root, `\Boot`, and `\Sources` directories.



[unreleased]: https://github.com/ipxe/wimboot/commits
[v2.7.0]: https://github.com/ipxe/wimboot/releases/tag/v2.7.0
[v2.6.0]: https://github.com/ipxe/wimboot/releases/tag/v2.6.0
[v2.5.2]: https://github.com/ipxe/wimboot/releases/tag/v2.5.2
[v2.5.1]: https://github.com/ipxe/wimboot/releases/tag/v2.5.1
[v2.5.0]: https://github.com/ipxe/wimboot/releases/tag/v2.5.0
[v2.4.1]: https://github.com/ipxe/wimboot/releases/tag/v2.4.1
[v2.4.0]: https://github.com/ipxe/wimboot/releases/tag/v2.4.0
[v2.3.0]: https://github.com/ipxe/wimboot/releases/tag/v2.3.0
[v2.2.5]: https://github.com/ipxe/wimboot/releases/tag/v2.2.5
[v2.2.4]: https://github.com/ipxe/wimboot/releases/tag/v2.2.4
[v2.2.3]: https://github.com/ipxe/wimboot/releases/tag/v2.2.3
[v2.2.2]: https://github.com/ipxe/wimboot/releases/tag/v2.2.2
[v2.2.1]: https://github.com/ipxe/wimboot/releases/tag/v2.2.1
[v2.2.0]: https://github.com/ipxe/wimboot/releases/tag/v2.2.0
[v2.1.0]: https://github.com/ipxe/wimboot/releases/tag/v2.1.0
[v2.0.0]: https://github.com/ipxe/wimboot/releases/tag/v2.0.0
[v1.0.6]: https://github.com/ipxe/wimboot/releases/tag/v1.0.6
[v1.0.5]: https://github.com/ipxe/wimboot/releases/tag/v1.0.5
[v1.0.4]: https://github.com/ipxe/wimboot/releases/tag/v1.0.4
[v1.0.3]: https://github.com/ipxe/wimboot/releases/tag/v1.0.3
[v1.0.2]: https://github.com/ipxe/wimboot/releases/tag/v1.0.2
[v1.0.1]: https://github.com/ipxe/wimboot/releases/tag/v1.0.1
[v1.0.0]: https://github.com/ipxe/wimboot/releases/tag/v1.0.0
[v0.9.3]: https://github.com/ipxe/wimboot/releases/tag/v0.9.3
[v0.9.2]: https://github.com/ipxe/wimboot/releases/tag/v0.9.2
[v0.9.1]: https://github.com/ipxe/wimboot/releases/tag/v0.9.1
[v0.9]: https://github.com/ipxe/wimboot/releases/tag/v0.9

[2pint]: https://2pintsoftware.com/
[actions]: https://github.com/ipxe/wimboot/actions
[coverity]: https://scan.coverity.com/
[digint]: https://digitalintelligence.com/
[jump]: https://jumptrading.com/
[travis]: https://travis-ci.com/
