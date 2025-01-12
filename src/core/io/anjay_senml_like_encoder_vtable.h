/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem Anjay LwM2M SDK
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#ifndef ANJAY_SENML_LIKE_ENCODER_VTABLE_H
#define ANJAY_SENML_LIKE_ENCODER_VTABLE_H

#include <anjay/core.h>

#include "anjay_senml_like_encoder.h"

VISIBILITY_PRIVATE_HEADER_BEGIN

typedef int (*senml_like_encode_uint_t)(anjay_senml_like_encoder_t *, uint64_t);
typedef int (*senml_like_encode_int_t)(anjay_senml_like_encoder_t *, int64_t);
typedef int (*senml_like_encode_double_t)(anjay_senml_like_encoder_t *, double);
typedef int (*senml_like_encode_bool_t)(anjay_senml_like_encoder_t *, bool);
typedef int (*senml_like_encode_string_t)(anjay_senml_like_encoder_t *,
                                          const char *);
typedef int (*senml_like_encode_objlnk_t)(anjay_senml_like_encoder_t *,
                                          const char *);
typedef int (*senml_like_element_begin_t)(anjay_senml_like_encoder_t *,
                                          const char *basename,
                                          const char *name,
                                          double time_s);
typedef int (*senml_like_element_end_t)(anjay_senml_like_encoder_t *);
typedef int (*senml_like_bytes_begin_t)(anjay_senml_like_encoder_t *, size_t);
typedef int (*senml_like_bytes_append_t)(anjay_senml_like_encoder_t *,
                                         const void *,
                                         size_t);
typedef int (*senml_like_bytes_end_t)(anjay_senml_like_encoder_t *);
typedef int (*senml_like_encoder_cleanup_t)(anjay_senml_like_encoder_t **);

typedef struct {
    senml_like_encode_uint_t senml_like_encode_uint;
    senml_like_encode_int_t senml_like_encode_int;
    senml_like_encode_double_t senml_like_encode_double;
    senml_like_encode_bool_t senml_like_encode_bool;
    senml_like_encode_string_t senml_like_encode_string;
    senml_like_encode_objlnk_t senml_like_encode_objlnk;
    senml_like_element_begin_t senml_like_element_begin;
    senml_like_element_end_t senml_like_element_end;
    senml_like_bytes_begin_t senml_like_bytes_begin;
    senml_like_bytes_append_t senml_like_bytes_append;
    senml_like_bytes_end_t senml_like_bytes_end;
    senml_like_encoder_cleanup_t senml_like_encoder_cleanup;
} anjay_senml_like_encoder_vtable_t;

struct anjay_senml_like_encoder_struct {
    const anjay_senml_like_encoder_vtable_t *vtable;
};

VISIBILITY_PRIVATE_HEADER_END

#endif // ANJAY_SENML_LIKE_ENCODER_VTABLE_H
