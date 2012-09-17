/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * @file
 *
 * Xpress Compression Algorithm (MS-XCA) decompression
 *
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "wimboot.h"
#include "xca.h"

/**
 * Construct XCA symbol table
 *
 * @v lengths		Huffman lengths tablee
 * @v sym		Symbol table to fill in
 * @ret rc		Return status code
 */
static int xca_symbols ( const struct xca_huf_len *lengths,
			 struct xca_symbols *sym ) {
	struct xca_huf_symbol *huf_sym;
	unsigned int huf;
	unsigned int raw;
	unsigned int len;
	unsigned int cum_freq;
	unsigned int adjustment;
	unsigned int first_byte;

	/* Zero symbol table */
	memset ( sym, 0, sizeof ( *sym ) );

	/* Count number of symbols with each Huffman-coded length */
	for ( raw = 0 ; raw < XCA_RAW_COUNT ; raw++ ) {
		len = xca_huf_len ( lengths, raw );
		if ( len ) {
			huf_sym = &sym->huf[ len - 1 ];
			huf_sym->freq++;
		}
	}

	/* Populate Huffman-coded symbol table */
	huf = 0;
	cum_freq = 0;
	for ( len = 1 ; len <= XCA_HUF_MAX_LEN ; len++ ) {
		huf_sym = &sym->huf[ len - 1 ];
		huf_sym->len = len;
		huf_sym->shift = ( XCA_HUF_MAX_LEN - len );
		huf_sym->start = ( huf << huf_sym->shift );
		huf_sym->raw = &sym->raw[cum_freq];
		huf += huf_sym->freq;
		if ( huf > ( 1U << len ) ) {
			DBG ( "Too many Huffman symbols with lengths <=%d\n",
			      len );
			return -1;
		}
		huf <<= 1;
		cum_freq += huf_sym->freq;
	}

	/* Populate raw symbol table */
	for ( raw = 0 ; raw < XCA_RAW_COUNT ; raw++ ) {
		len = xca_huf_len ( lengths, raw );
		if ( len ) {
			huf_sym = &sym->huf[ len - 1 ];
			*(huf_sym->raw++) = raw;
		}
	}

	/* Adjust Huffman-coded symbol table raw pointers */
	for ( len = 1 ; len <= XCA_HUF_MAX_LEN ; len++ ) {
		huf_sym = &sym->huf[ len - 1 ];
		huf_sym->raw -= huf_sym->freq; /* Reset to first symbol */
		adjustment = ( huf_sym->start >> huf_sym->shift );
		huf_sym->raw -= adjustment; /* Adjust for quick indexing */
	}

	/* Populate first byte lookup table */
	for ( len = 1 ; len <= XCA_HUF_MAX_LEN ; len++ ) {
		huf_sym = &sym->huf[ len - 1 ];
		for ( first_byte = ( huf_sym->start >>
				     ( XCA_HUF_MAX_LEN - 8 ) ) ;
		      first_byte < 256 ; first_byte++ ) {
			sym->max_len[first_byte] = len;
		}

	}

	return 0;
}

/**
 * Dump XCA symbol table (for debugging)
 *
 * @v sym		Symbol table
 */
static void __attribute__ (( unused )) xca_dump ( struct xca_symbols *sym ) {
	struct xca_huf_symbol *huf_sym;
	unsigned int len;
	unsigned int huf_start;
	unsigned int huf_end;
	unsigned int huf;

	/* Dump symbols for each length */
	for ( len = 1 ; len <= XCA_HUF_MAX_LEN ; len++ ) {
		huf_sym = &sym->huf[ len - 1 ];
		printf ( "Length %d: start %04x:", len, huf_sym->start );
		huf_start = ( huf_sym->start >> huf_sym->shift );
		huf_end = ( huf_start + huf_sym->freq );
		for ( huf = huf_start ; huf < huf_end ; huf++ )
			printf ( " %03x", huf_sym->raw[huf] );
		printf ( "\n" );
	}
}

/**
 * Decode XCA Huffman-coded symbol
 *
 * @v sym		Symbol table
 * @v huf_max		Huffman-coded symbol (normalised to maximum length)
 * @ret huf_sym		Huffman-coded symbol set
 */
static struct xca_huf_symbol * xca_decode ( struct xca_symbols *sym,
					    unsigned int huf_max ) {
	struct xca_huf_symbol *huf_sym;
	unsigned int first_byte;

	/* Identify length by scanning table */
	first_byte = ( huf_max >> ( XCA_HUF_MAX_LEN - 8 ) );
	huf_sym = &sym->huf[ sym->max_len[first_byte] - 1 ];
	while ( 1 ) {
		if ( likely ( huf_max >= huf_sym->start ) )
			return huf_sym;
		huf_sym--;
	}
}

/**
 * Dump XCA Huffman decoding result (for debugging)
 *
 * @v sym		Symbol table
 * @v huf_max		Huffman-coded symbol (normalised to maximum length)
 */
static void __attribute__ (( unused ))
xca_decode_dump ( struct xca_symbols *sym, unsigned int huf_max ) {
	struct xca_huf_symbol *huf_sym;
	unsigned int raw;

	/* Decode symbol */
	huf_sym = xca_decode ( sym, huf_max );

	/* Determine raw symbol */
	raw = huf_sym->raw[ huf_max >> huf_sym->shift ];

	/* Look up raw symbol */
	printf ( "Decoded %04x to length %d value %03x\n",
		 huf_max, huf_sym->len, raw );
}

/**
 * Decompress XCA-compressed data
 *
 * @v data		Compressed data
 * @v len		Length of compressed data
 * @v buf		Decompression buffer, or NULL
 * @ret out_len		Length of decompressed data, or negative error
 */
ssize_t xca_decompress ( const void *data, size_t len, void *buf ) {
	const void *src = data;
	const void *end = ( src + len );
	uint8_t *out = buf;
	size_t out_len = 0;
	const struct xca_huf_len *lengths;
	struct xca_symbols sym;
	uint32_t accum = 0;
	int extra_bits = 0;
	unsigned int huf_max;
	struct xca_huf_symbol *huf_sym;
	unsigned int raw;
	unsigned int match_len;
	unsigned int match_offset_bits;
	unsigned int match_offset;
	const uint8_t *copy;
	int rc;

	/* Process data stream */
	while ( src < end ) {

		/* (Re)initialise decompressor if applicable */
		if ( ( out_len % XCA_BLOCK_SIZE ) == 0 ) {

			/* Construct symbol table */
			lengths = src;
			src += sizeof ( *lengths );
			if ( src > end ) {
				DBG ( "XCA too short to hold Huffman lengths "
				      "table at input offset %#zx\n",
				      ( src - data ) );
				return -1;
			}
			if ( ( rc = xca_symbols ( lengths, &sym ) ) != 0 )
				return rc;

			/* Initialise state */
			accum = XCA_GET16 ( src );
			accum <<= 16;
			accum |= XCA_GET16 ( src );
			extra_bits = 16;
		}

		/* Determine symbol */
		huf_max = ( accum >> ( 32 - XCA_HUF_MAX_LEN ) );
		huf_sym = xca_decode ( &sym, huf_max );
		raw = huf_sym->raw[ huf_max >> huf_sym->shift ];
		accum <<= huf_sym->len;
		extra_bits -= huf_sym->len;
		if ( extra_bits < 0 ) {
			accum |= ( XCA_GET16 ( src ) << ( -extra_bits ) );
			extra_bits += 16;
		}

		/* Process symbol */
		if ( raw < XCA_END_MARKER ) {

			/* Literal symbol - add to output stream */
			if ( buf )
				*(out++) = raw;
			out_len++;

		} else if ( ( raw == XCA_END_MARKER ) &&
			    ( src >= ( end - 1 ) ) ) {

			/* End marker symbol */
			return out_len;

		} else {

			/* LZ77 match symbol */
			raw -= XCA_END_MARKER;
			match_offset_bits = ( raw >> 4 );
			match_len = ( raw & 0x0f );
			if ( match_len == 0x0f ) {
				match_len = XCA_GET8 ( src );
				if ( match_len == 0xff ) {
					match_len = XCA_GET16 ( src );
				} else {
					match_len += 0x0f;
				}
			}
			match_len += 3;
			if ( match_offset_bits ) {
				match_offset =
					( ( accum >> ( 32 - match_offset_bits ))
					  + ( 1 << match_offset_bits ) );
			} else {
				match_offset = 1;
			}
			accum <<= match_offset_bits;
			extra_bits -= match_offset_bits;
			if ( extra_bits < 0 ) {
				accum |= ( XCA_GET16 ( src ) << (-extra_bits) );
				extra_bits += 16;
			}

			/* Copy data */
			out_len += match_len;
			if ( buf ) {
				copy = ( out - match_offset );
				while ( match_len-- )
					*(out++) = *(copy++);
			}
		}
	}

	DBG ( "XCA input overrun at output length %#zx\n", out_len );
	return -1;
}
