/*
 * kk_ihex_read.h: A simple library for reading Intel HEX data. See
 * the accompanying kk_ihex_write.h for IHEX write support.
 *
 *
 *      READING INTEL HEX DATA
 *      ----------------------
 *
 * To read data in the Intel HEX format, you must perform the actual reading
 * of bytes using other means (e.g., stdio). The bytes read must then be
 * passed to `ihex_read_byte` and/or `ihex_read_bytes`. The reading functions
 * will then call `ihex_data_read`, at which stage the `struct ihex_state`
 * structure will contain the data along with its address. See below for
 * details and example implementation of `ihex_data_read`.
 *
 * The sequence to read data in IHEX format is:
 *      struct ihex_state ihex;
 *      ihex_begin_read(&ihex);
 *      ihex_read_bytes(&ihex, my_input_bytes, length_of_my_input_bytes);
 *      ihex_end_read(&ihex);
 *
 *
 *      CONSERVING MEMORY
 *      -----------------
 *
 * For memory-critical use, you can save additional memory by defining
 * `IHEX_LINE_MAX_LENGTH` as something less than 255. Note, however, that
 * this limit affects both reading and writing, so the resulting library
 * will be unable to read lines with more than this number of data bytes.
 * That said, I haven't encountered any IHEX files with more than 32
 * data bytes per line.
 *
 *
 * Copyright (c) 2013-2015 Kimmo Kulovesi, http://arkku.com/
 * Provided with absolutely no warranty, use at your own risk only.
 * Use and distribute freely, mark modified copies as such.
 *
 * Modifications Copyright (c) 2015 National Instruments Corp.
 */

#ifndef KK_IHEX_READ_H
#define KK_IHEX_READ_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kk_ihex.h"

#include <stdio.h>

// Begin reading at address 0
void ihex_begin_read(struct ihex_state * const ihex);

// Begin reading at `address` (the lowest 16 bits of which will be ignored);
// this is required only if the high bytes of the 32-bit starting address
// are not specified in the input data and they are non-zero
void ihex_read_at_address(struct ihex_state *ihex,
                          ihex_address_t address);

// Read a single character
void ihex_read_byte(struct ihex_state *ihex, char chr, FILE* outfile);

// Read `count` bytes from `data`
void ihex_read_bytes(struct ihex_state * ihex,
                     const char * data,
                     ihex_count_t count,
                     FILE* outfile);

// End reading (may call `ihex_data_read` if there is data waiting)
void ihex_end_read(struct ihex_state *ihex, FILE* outfile);

// Called when a complete line has been read, the record type of which is
// passed as `type`. The `ihex` structure will have its fields `data`,
// `line_length`, `address`, and `segment` set appropriately. In case
// of reading an `IHEX_EXTENDED_LINEAR_ADDRESS_RECORD` or an
// `IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD` the record's data is not
// yet parsed - it will be parsed into the `address` or `segment` field
// only if `ihex_data_read` returns `true`. This allows manual handling
// of extended addresses by parsing the `ihex->data` bytes.
//
// Possible error cases include checksum mismatch (which is indicated
// as an argument), and excessive line length (in case this has been
// compiled with `IHEX_LINE_MAX_LENGTH` less than 255) which is indicated
// by `line_length` greater than `length`. Unknown record types and
// other erroneous data is usually silently ignored by this minimalistic
// parser. (It is recommended to compute a hash over the complete data
// once received and verify that against the source.)
//
// Example implementation:
//
//      ihex_bool_t ihex_data_read(struct ihex_state *ihex,
//                                 ihex_record_type_t type,
//                                 ihex_bool_t error) {
//          error = error || (ihex->length < ihex->line_length);
//          if (type == IHEX_DATA_RECORD && !error) {
//              (void) fseek(outfile, IHEX_LINEAR_ADDRESS(ihex), SEEK_SET);
//              (void) fwrite(ihex->data, 1, ihex->length, outfile);
//          } else if (type == IHEX_END_OF_FILE_RECORD) {
//              (void) fclose(outfile);
//          }
//          return !error;
//      }
//
ihex_bool_t ihex_data_read(struct ihex_state *ihex,
                           ihex_record_type_t type,
                           ihex_bool_t checksum_mismatch,
                           FILE* outfile);

// Begin reading at `segment`; this is required only if the initial segment
// is not specified in the input data and it is non-zero.
//
#ifndef IHEX_DISABLE_SEGMENTS
void ihex_read_at_segment(struct ihex_state *ihex, ihex_segment_t segment);
#endif

#ifdef __cplusplus
}
#endif
#endif // !KK_IHEX_READ_H
