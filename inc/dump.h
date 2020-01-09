/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

/**
 * @file dump.h
 * @author Robert Schmidt
 * @brief Simulator state dump and helper functions.
 *
 * @remark The state of the simulation is serialized to JSON,
 * which allows to pick up and resume simulations after a deadline miss.
 */

#pragma once
#include <stdio.h>
#include "json.h"

#ifndef DUMP_BUFLEN
#define DUMP_BUFLEN 128
#endif

/**
 * @brief Append two arrays in new buffer.
 *
 * Both array @p a and @p b are copied to a new allocated buffer.
 *
 * @return Handle to new buffer.
 */
void** merge(void** a, void** b, int na, int nb);

/**
 * @brief Returns number of unique elements copied from @p src to @p dst.
 *
 * @p dst and @p src are both arrays of same length @p n.
 *
 * @warning If memory allocation fails, function returns zero!
 *
 * @return Number of unique elements
 */
int uniq(void* const* const src, void** dst, int n);

/**
 * @brief Initialize dump of JSON to stream.
 * @see json.h from libjson
 */
json_printer* dump_json_tostream_init(FILE* stream);

/**
 * @brief Free buffers used during dump.
 * @see json.h from libjson
 */
void dump_json_tostream_free(json_printer* p);

/**
 * @brief Actual dump.
 * @see json.h from libjson
 */
void dump_json_tostream(json_printer* p, int64_t const data);

/* Ease reading of integers from JSON */
/**
 * @brief Initialize read of JSON simulator state dump to buffer.
 * @see json.h from libjson
 */
json_parser* dump_read_json_init(void* cb_data);

/**
 * @brief Actual reading and parsing of JSON simulator state dump.
 * @see json.h from libjson
 */
void dump_read_json_parse(json_parser* p, FILE* stream);

/**
 * @brief Free buffers used during read.
 * @see json.h from libjson
 */
void dump_read_json_free(json_parser* p);
