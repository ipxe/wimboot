SECTIONS {

	/* Align sections to keep PE tools happy */
	alignment = 512;

	/* Virtual addresses start at 0x20000 */
	. = 0x20000;
	_start = .;

	/* bzImage prefix */
	_prefix_pos = 0;
	.prefix : AT ( _prefix_pos ) {
		_prefix = .;
		*(.prefix)
		*(.prefix.*)
		. = ALIGN ( alignment );
		_eprefix = .;
	}
	_prefix_len = ABSOLUTE ( _eprefix ) - ABSOLUTE ( _prefix );

	/* Real-mode uninitialised data section */
	.bss16 ( NOLOAD ) : {
		_bss16 = .;
		*(.stack16)
		*(.stack16.*)
		*(.bss16)
		*(.bss16.*)
		. = ALIGN ( alignment );
		_ebss16 = .;
	}
	_bss16_len = ABSOLUTE ( _ebss16 ) - ABSOLUTE ( _bss16 );

	/* Data (and 16-bit BIOS text) section */
	_data_pos = ( _prefix_pos + _prefix_len );
	.data : AT ( _data_pos ) {
		_data = .;
		_payload = .;
		/* Portions that must be accessible in 16-bit modes */
		*(.text16)
		*(.text16.*)
		*(.rodata16)
		*(.rodata16.*)
		*(.data16)
		*(.data16.*)
		/* Portions that need not be accessible in 16-bit modes */
		*(.rodata)
		*(.rodata.*)
		*(.data)
		*(.data.*)
		. = ALIGN ( alignment );
		_edata = .;
	}
	_data_len = ABSOLUTE ( _edata ) - ABSOLUTE ( _data );

	/* Text (excluding 16-bit BIOS text) section */
	_text_pos = ( _data_pos + _data_len );
	.text : AT ( _text_pos ) {
		_text = .;
		*(.text)
		*(.text.*)
		. = ALIGN ( alignment );
		_epayload = .;
		_etext = .;
	}
	_text_len = ABSOLUTE ( _etext ) - ABSOLUTE ( _text );
	_payload_len = ABSOLUTE ( _epayload ) - ABSOLUTE ( _payload );

	/* bootmgr.exe hardcodes the address 0x30000 for use as a
	 * buffer accessible by real-mode code.  We can't fit our
	 * .text, .data, and .bss below this region, so explicitly
	 * place the .bss higher in memory.
	 */
	_forbidden_start = 0x30000;
	_forbidden_end = 0x40000;

	/* Uninitialised data section */
	.bss ( NOLOAD ) : {
		_bss = .;
		ASSERT ( ABSOLUTE ( . ) <= ABSOLUTE ( _forbidden_start ),
			 "Binary is too large" );
		. = ABSOLUTE ( _forbidden_end );
		*(.bss)
		*(.bss.*)
		*(COMMON)
		*(.stack)
		*(.stack.*)
		. = ALIGN ( alignment );
		_ebss = .;
	}
	_bss_len = ABSOLUTE ( _ebss ) - ABSOLUTE ( _bss );

	/* Secure Boot Advanced Targeting (SBAT) section */
	_sbat_pos = ( _text_pos + _text_len );
	.sbat : AT ( _sbat_pos ) {
		_sbat = .;
		*(.sbat)
		*(.sbat.*)
		_msbat = .;
		. = ALIGN ( alignment );
		_esbat = .;
	}
	_sbat_used = ABSOLUTE ( _msbat ) - ABSOLUTE ( _sbat );
	_sbat_len = ABSOLUTE ( _esbat ) - ABSOLUTE ( _sbat );

	/* Relocations section */
	_reloc_pos = ( _sbat_pos + _sbat_len );
	_reloc = .;

	_end = .;

	/* Symbols required by i386.x86_64 objects */
	__i386__start = _start;
	__i386__end = _end;

	/DISCARD/ : {
		*(.comment)
		*(.comment.*)
		*(.note)
		*(.note.*)
		*(.eh_frame)
		*(.eh_frame.*)
		*(.rel)
		*(.rel.*)
	}
}
