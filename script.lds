SECTIONS {

	/* bzImage prefix */
	.prefix : AT ( 0 ) {
		_prefix = .;
		*(.prefix)
		*(.prefix.*)
		_eprefix = .;
	}
	_prefix_filesz = ABSOLUTE ( _eprefix ) - ABSOLUTE ( _prefix );

	/* Payload code executes at 0x20000 */
	. = 0x20000;

	/* Payload section */
	.payload : AT ( _prefix_filesz ) {
		_payload = .;
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		*(.data)
		*(.data.*)
		_epayload = .;
	}
	_payload_filesz = ABSOLUTE ( _epayload ) - ABSOLUTE ( _payload );

	/* Uninitialised data section */
	.bss ( NOLOAD ) : {
		*(.bss)
		*(.bss.*)
	}
}
