/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem Anjay LwM2M SDK
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#ifndef ANJAY_INCLUDE_ANJAY_MODULES_IO_UTILS_H
#define ANJAY_INCLUDE_ANJAY_MODULES_IO_UTILS_H

#include <stdint.h>

#include <avsystem/commons/avs_stream.h>

#include <anjay/io.h>

#include <anjay_modules/anjay_dm_utils.h>
#include <anjay_modules/anjay_raw_buffer.h>

VISIBILITY_PRIVATE_HEADER_BEGIN

typedef int anjay_input_ctx_constructor_t(anjay_unlocked_input_ctx_t **out,
                                          avs_stream_t **stream_ptr,
                                          const anjay_uri_path_t *request_uri);

anjay_input_ctx_constructor_t _anjay_input_opaque_create;

#ifndef ANJAY_WITHOUT_PLAINTEXT
anjay_input_ctx_constructor_t _anjay_input_text_create;
#endif // ANJAY_WITHOUT_PLAINTEXT

#ifndef ANJAY_WITHOUT_TLV
anjay_input_ctx_constructor_t _anjay_input_tlv_create;
#endif // ANJAY_WITHOUT_TLV

#ifdef ANJAY_WITH_CBOR
anjay_input_ctx_constructor_t _anjay_input_cbor_create;
anjay_input_ctx_constructor_t _anjay_input_senml_cbor_create;
anjay_input_ctx_constructor_t _anjay_input_senml_cbor_composite_read_create;

#endif // ANJAY_WITH_CBOR

#ifdef ANJAY_WITH_SENML_JSON
anjay_input_ctx_constructor_t _anjay_input_json_create;
anjay_input_ctx_constructor_t _anjay_input_json_composite_read_create;
#endif // ANJAY_WITH_SENML_JSON

int _anjay_input_ctx_destroy(anjay_unlocked_input_ctx_t **ctx_ptr);

/**
 * Fetches bytes from @p ctx. On success it frees underlying @p buffer storage
 * via @p _anjay_sec_raw_buffer_clear and reinitializes @p buffer properly with
 * obtained data.
 */
int _anjay_io_fetch_bytes(anjay_unlocked_input_ctx_t *ctx,
                          anjay_raw_buffer_t *buffer);

/**
 * Fetches string from @p ctx. It calls avs_free() on @p *out and, on success,
 * reinitializes @p *out properly with a pointer to (heap allocated) obtained
 * data.
 */
int _anjay_io_fetch_string(anjay_unlocked_input_ctx_t *ctx, char **out);

VISIBILITY_PRIVATE_HEADER_END

#endif /* ANJAY_INCLUDE_ANJAY_MODULES_IO_UTILS_H */
