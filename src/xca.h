#ifndef _XCA_H
#define _XCA_H

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

/** Maximum length (in bits) of a raw symbol */
#define XCA_RAW_MAX_LEN 9

/** Number of raw symbols */
#define XCA_RAW_COUNT ( 1 << XCA_RAW_MAX_LEN )

/** Type capable of holding a maximum-length raw symbol */
typedef uint16_t xca_raw_t;

/** Maximum length (in bits) of a Huffman-coded symbol */
#define XCA_HUF_MAX_LEN 15

/** An XCA Huffman-coded symbol set of a given length */
struct xca_huf_symbol {
	/** Length */
	uint8_t len;
	/** Shift */
	uint8_t shift;
	/** First Huffman-coded symbol having this length
	 *
	 * Normalised to be the length of a maximum-length
	 * Huffman-coded symbol.
	 */
	unsigned int start;
	/** Number of Huffman-coded symbols having this length */
	unsigned int freq;
	/** Raw symbols having this length when Huffman-coded
	 *
	 * This is a pointer into the XCA raw symbol table, adjusted
	 * so that the Huffman-coded symbol can be right-shifted and
	 * used directly as an array index.
	 */
	xca_raw_t *raw;
};

/** XCA symbol table */
struct xca_symbols {
	/** Huffman-coded symbol set for each length */
	struct xca_huf_symbol huf[XCA_HUF_MAX_LEN];
	/** Raw symbols
	 *
	 * Ordered by Huffman-coded symbol length, then by symbol value.
	 */
	xca_raw_t raw[XCA_RAW_COUNT];
	/** First byte lookup table */
	uint8_t max_len[256];
};

/** XCA symbol Huffman lengths table */
struct xca_huf_len {
	/** Lengths of each symbol */
	uint8_t nibbles[ XCA_RAW_COUNT / 2 ];
} __attribute__ (( packed ));

/**
 * Extract Huffman-coded length of a raw symbol
 *
 * @v lengths		Huffman lengths table
 * @v symbol		Raw symbol
 * @ret len		Huffman-coded length
 */
static inline unsigned int xca_huf_len ( const struct xca_huf_len *lengths,
					 unsigned int symbol ) {
	return ( ( ( lengths->nibbles[ symbol / 2 ] ) >>
		   ( 4 * ( symbol % 2 ) ) ) & 0x0f );
}

/** Get word from source data stream */
#define XCA_GET16( src ) ( {			\
	const uint16_t *src16 = src;		\
	src += sizeof ( *src16 );		\
	*src16; } )

/** Get byte from source data stream */
#define XCA_GET8( src ) ( {			\
	const uint8_t *src8 = src;		\
	src += sizeof ( *src8 );		\
	*src8; } )

/** XCA source data stream end marker */
#define XCA_END_MARKER 256

/** XCA block size */
#define XCA_BLOCK_SIZE ( 64 * 1024 )

extern ssize_t xca_decompress ( const void *data, size_t len, void *buf );

#endif /* _XCA_H */

