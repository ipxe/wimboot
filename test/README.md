Automated tests
===============

Windows images
--------------

To run the automated tests, you will need to create a set of Windows
boot images:

- `./images/win7/x64` - Windows 7 (64-bit)
- `./images/win7/x86` - Windows 7 (32-bit)
- `./images/win8/x64` - Windows 8.1 (64-bit)
- `./images/win8/x86` - Windows 8.1 (32-bit)
- `./images/win10/x64` - Windows 10 (64-bit)
- `./images/win10/x86` - Windows 10 (32-bit)

Within each of the above directories, you need to provide the files:

- `bootmgr`
- `boot/bcd`
- `boot/boot.sdi`
- `sources/boot.wim`

You can find these files within the relevant Windows installation
`.iso` images.

For example, you can extract those files with:

```
7z x \
    -oimages/win7/x64 \
    your-windows-7.iso \
    bootmgr \
    boot/bcd \
    boot/boot.sdi \
    sources/boot.wim
```

Running tests
-------------

Each test is described in a `.yml` file.  You can run tests using
`./testwimboot`, e.g.

```
./testwimboot -v -i win7.yml
```

At the end of each test run, the `out` directory will include a debug
`.log` and a final screenshot `.png` for the test.

iPXE boot ROM
-------------

Most Linux distributions provide iPXE boot ROMs for BIOS virtual
machines, but may not provide a suitable boot ROM for UEFI virtual
machines.

You can build your own iPXE boot ROM as follows:

```
git clone https://github.com/ipxe/ipxe.git
make -C ipxe/src bin/8086100e.rom bin-x86_64-efi/8086100e.efirom
./ipxe/src/util/catrom.pl ipxe/src/bin/8086100e.rom \
    ipxe/src/bin-x86_64-efi/8086100e.efirom > images/e1000.rom
```

This will produce a combined ROM image that can be used for both BIOS
and UEFI virtual machines.

You will need to use the `-r` option to use this ROM, e.g.:

```
./testwimboot -r images/e1000.rom -v -i win7_uefi.yml
```
