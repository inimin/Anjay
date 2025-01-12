/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem Anjay LwM2M SDK
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#include <anjay_init.h>

#include <inttypes.h>
#include <stdlib.h>

#include <avsystem/commons/avs_memory.h>
#include <avsystem/commons/avs_stream_v_table.h>
#include <avsystem/commons/avs_utils.h>

#include <anjay/core.h>

#include "coap/anjay_content_format.h"

#include "anjay_io_core.h"
#include "io/anjay_vtable.h"

VISIBILITY_SOURCE_BEGIN

struct anjay_unlocked_dm_list_ctx_struct {
    const anjay_dm_list_ctx_vtable_t *vtable;
};

void _anjay_dm_emit_unlocked(anjay_unlocked_dm_list_ctx_t *ctx, uint16_t id) {
    assert(ctx->vtable && ctx->vtable->emit);
    ctx->vtable->emit(ctx, id);
}

void anjay_dm_emit(anjay_dm_list_ctx_t *ctx, uint16_t id) {
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    _anjay_dm_emit_unlocked(_anjay_dm_list_get_unlocked(ctx), id);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
}

struct anjay_unlocked_ret_bytes_ctx_struct {
    const anjay_ret_bytes_ctx_vtable_t *vtable;
};

#ifdef ANJAY_WITH_LEGACY_CONTENT_FORMAT_SUPPORT

uint16_t _anjay_translate_legacy_content_format(uint16_t format) {
    static const char MSG_FMT[] =
            "legacy application/vnd.oma.lwm2m+%s Content-Format value: %d";

    switch (format) {
    case ANJAY_COAP_FORMAT_LEGACY_PLAINTEXT:
        anjay_log(DEBUG, MSG_FMT, _("text"),
                  ANJAY_COAP_FORMAT_LEGACY_PLAINTEXT);
        return AVS_COAP_FORMAT_PLAINTEXT;

    case ANJAY_COAP_FORMAT_LEGACY_TLV:
        anjay_log(DEBUG, MSG_FMT, _("tlv"), ANJAY_COAP_FORMAT_LEGACY_TLV);
        return AVS_COAP_FORMAT_OMA_LWM2M_TLV;

    case ANJAY_COAP_FORMAT_LEGACY_JSON:
        anjay_log(DEBUG, MSG_FMT, _("json"), ANJAY_COAP_FORMAT_LEGACY_JSON);
        return AVS_COAP_FORMAT_OMA_LWM2M_JSON;

    case ANJAY_COAP_FORMAT_LEGACY_OPAQUE:
        anjay_log(DEBUG, MSG_FMT, _("opaque"), ANJAY_COAP_FORMAT_LEGACY_OPAQUE);
        return AVS_COAP_FORMAT_OCTET_STREAM;

    default:
        return format;
    }
}

#endif // ANJAY_WITH_LEGACY_CONTENT_FORMAT_SUPPORT

anjay_unlocked_ret_bytes_ctx_t *
_anjay_ret_bytes_begin_unlocked(anjay_unlocked_output_ctx_t *ctx,
                                size_t length) {
    anjay_unlocked_ret_bytes_ctx_t *bytes_ctx = NULL;
    int result = _anjay_output_bytes_begin(ctx, length, &bytes_ctx);
    assert(!result == !!bytes_ctx);
    (void) result;
    return bytes_ctx;
}

anjay_ret_bytes_ctx_t *anjay_ret_bytes_begin(anjay_output_ctx_t *ctx,
                                             size_t length) {
#ifdef ANJAY_WITH_THREAD_SAFETY
    anjay_ret_bytes_ctx_t *retval = NULL;
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    anjay_unlocked_ret_bytes_ctx_t *bytes_ctx =
            _anjay_ret_bytes_begin_unlocked(_anjay_output_get_unlocked(ctx),
                                            length);
#ifdef ANJAY_WITH_THREAD_SAFETY
    if (bytes_ctx) {
        ctx->bytes_ctx.unlocked_ctx = bytes_ctx;
        retval = &ctx->bytes_ctx;
    }
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
    return retval;
#else  // ANJAY_WITH_THREAD_SAFETY
    return (anjay_ret_bytes_ctx_t *) bytes_ctx;
#endif // ANJAY_WITH_THREAD_SAFETY
}

int _anjay_ret_bytes_append_unlocked(anjay_unlocked_ret_bytes_ctx_t *ctx,
                                     const void *data,
                                     size_t length) {
    assert(ctx && ctx->vtable && ctx->vtable->append);
    return ctx->vtable->append(ctx, data, length);
}

int anjay_ret_bytes_append(anjay_ret_bytes_ctx_t *ctx,
                           const void *data,
                           size_t length) {
    assert(ctx);
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    anjay_t *anjay_locked =
            AVS_CONTAINER_OF(ctx, anjay_output_ctx_t, bytes_ctx)->anjay_locked;
    ANJAY_MUTEX_LOCK(anjay, anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result =
            _anjay_ret_bytes_append_unlocked(_anjay_ret_bytes_get_unlocked(ctx),
                                             data, length);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_ret_bytes_unlocked(anjay_unlocked_output_ctx_t *ctx,
                              const void *data,
                              size_t length) {
    anjay_unlocked_ret_bytes_ctx_t *bytes =
            _anjay_ret_bytes_begin_unlocked(ctx, length);
    if (!bytes) {
        return -1;
    } else {
        return _anjay_ret_bytes_append_unlocked(bytes, data, length);
    }
}

int anjay_ret_bytes(anjay_output_ctx_t *ctx, const void *data, size_t length) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_bytes_unlocked(_anjay_output_get_unlocked(ctx), data,
                                       length);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_ret_string_unlocked(anjay_unlocked_output_ctx_t *ctx,
                               const char *value) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->string) {
        result = ctx->vtable->string(ctx, value);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int anjay_ret_string(anjay_output_ctx_t *ctx, const char *value) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_string_unlocked(_anjay_output_get_unlocked(ctx), value);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_ret_i64_unlocked(anjay_unlocked_output_ctx_t *ctx, int64_t value) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->integer) {
        result = ctx->vtable->integer(ctx, value);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int anjay_ret_i64(anjay_output_ctx_t *ctx, int64_t value) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_i64_unlocked(_anjay_output_get_unlocked(ctx), value);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

#ifdef ANJAY_WITH_LWM2M11
int _anjay_ret_u64_unlocked(anjay_unlocked_output_ctx_t *ctx, uint64_t value) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->uint) {
        result = ctx->vtable->uint(ctx, value);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int anjay_ret_u64(anjay_output_ctx_t *ctx, uint64_t value) {
    int result = -1;
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_u64_unlocked(_anjay_output_get_unlocked(ctx), value);
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}
#endif // ANJAY_WITH_LWM2M11

int _anjay_ret_double_unlocked(anjay_unlocked_output_ctx_t *ctx, double value) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->floating) {
        result = ctx->vtable->floating(ctx, value);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int anjay_ret_double(anjay_output_ctx_t *ctx, double value) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_double_unlocked(_anjay_output_get_unlocked(ctx), value);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_ret_bool_unlocked(anjay_unlocked_output_ctx_t *ctx, bool value) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->boolean) {
        result = ctx->vtable->boolean(ctx, value);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int anjay_ret_bool(anjay_output_ctx_t *ctx, bool value) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_bool_unlocked(_anjay_output_get_unlocked(ctx), value);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_ret_objlnk_unlocked(anjay_unlocked_output_ctx_t *ctx,
                               anjay_oid_t oid,
                               anjay_iid_t iid) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->objlnk) {
        result = ctx->vtable->objlnk(ctx, oid, iid);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int anjay_ret_objlnk(anjay_output_ctx_t *ctx,
                     anjay_oid_t oid,
                     anjay_iid_t iid) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_objlnk_unlocked(_anjay_output_get_unlocked(ctx), oid,
                                        iid);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

#if defined(ANJAY_WITH_SECURITY_STRUCTURED)
int _anjay_ret_security_info_unlocked(
        anjay_unlocked_output_ctx_t *ctx,
        const avs_crypto_security_info_union_t *desc) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->security_info) {
        result = ctx->vtable->security_info(ctx, desc);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}
#endif /* defined(ANJAY_WITH_SECURITY_STRUCTURED) || \
          defined(ANJAY_WITH_MODULE_SECURITY_ENGINE_SUPPORT) */

#ifdef ANJAY_WITH_SECURITY_STRUCTURED
int anjay_ret_certificate_chain_info(
        anjay_output_ctx_t *ctx,
        avs_crypto_certificate_chain_info_t certificate_chain_info) {
    assert(certificate_chain_info.desc.type
           == AVS_CRYPTO_SECURITY_INFO_CERTIFICATE_CHAIN);
    int result = -1;
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_security_info_unlocked(_anjay_output_get_unlocked(ctx),
                                               &certificate_chain_info.desc);
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int anjay_ret_private_key_info(anjay_output_ctx_t *ctx,
                               avs_crypto_private_key_info_t private_key_info) {
    int result = -1;
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_security_info_unlocked(_anjay_output_get_unlocked(ctx),
                                               &private_key_info.desc);
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int anjay_ret_psk_identity_info(
        anjay_output_ctx_t *ctx,
        avs_crypto_psk_identity_info_t psk_identity_info) {
    int result = -1;
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_security_info_unlocked(_anjay_output_get_unlocked(ctx),
                                               &psk_identity_info.desc);
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int anjay_ret_psk_key_info(anjay_output_ctx_t *ctx,
                           avs_crypto_psk_key_info_t psk_key_info) {
    int result = -1;
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_ret_security_info_unlocked(_anjay_output_get_unlocked(ctx),
                                               &psk_key_info.desc);
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}
#endif // ANJAY_WITH_SECURITY_STRUCTURED

int _anjay_output_bytes_begin(anjay_unlocked_output_ctx_t *ctx,
                              size_t length,
                              anjay_unlocked_ret_bytes_ctx_t **out_bytes_ctx) {
    assert(out_bytes_ctx);
    assert(!*out_bytes_ctx);
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->bytes_begin) {
        result = ctx->vtable->bytes_begin(ctx, length, out_bytes_ctx);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int _anjay_output_start_aggregate(anjay_unlocked_output_ctx_t *ctx) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->start_aggregate) {
        result = ctx->vtable->start_aggregate(ctx);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int _anjay_output_set_path(anjay_unlocked_output_ctx_t *ctx,
                           const anjay_uri_path_t *path) {
    // NOTE: We explicitly consider NULL set_path() to be always successful,
    // to simplify implementation of outbuf_ctx and the like.
    int result = 0;
    if (ctx->vtable->set_path) {
        result = ctx->vtable->set_path(ctx, path);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int _anjay_output_clear_path(anjay_unlocked_output_ctx_t *ctx) {
    int result = ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED;
    if (ctx->vtable->clear_path) {
        result = ctx->vtable->clear_path(ctx);
    }
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int _anjay_output_set_time(anjay_unlocked_output_ctx_t *ctx, double value) {
    if (!ctx->vtable->set_time) {
        // Deliberately ignore set_time fails - non-SenML formats will just omit
        // the timestamps, this is fine.
        return 0;
    }
    int result = ctx->vtable->set_time(ctx, value);
    _anjay_update_ret(&ctx->error, result);
    return result;
}

int _anjay_output_ctx_destroy(anjay_unlocked_output_ctx_t **ctx_ptr) {
    anjay_unlocked_output_ctx_t *ctx = *ctx_ptr;
    int result = 0;
    if (ctx) {
        result = ctx->error;
        if (ctx->vtable->close) {
            _anjay_update_ret(&result, ctx->vtable->close(ctx));
        }
        avs_free(ctx);
        *ctx_ptr = NULL;
    }
    return result;
}

int _anjay_output_ctx_destroy_and_process_result(
        anjay_unlocked_output_ctx_t **out_ctx_ptr, int result) {
    int destroy_result = _anjay_output_ctx_destroy(out_ctx_ptr);
    if (destroy_result != ANJAY_OUTCTXERR_ANJAY_RET_NOT_CALLED) {
        return destroy_result ? destroy_result : result;
    } else if (result) {
        return result;
    } else {
        anjay_log(ERROR,
                  _("unable to determine resource type: anjay_ret_* not called "
                    "during successful resource_read handler call"));
        return ANJAY_ERR_INTERNAL;
    }
}

static int get_some_bytes(anjay_unlocked_input_ctx_t *ctx,
                          size_t *out_bytes_read,
                          bool *out_message_finished,
                          void *out_buf,
                          size_t buf_size) {
    if (!ctx->vtable->some_bytes) {
        return -1;
    }
    return ctx->vtable->some_bytes(ctx, out_bytes_read, out_message_finished,
                                   out_buf, buf_size);
}

int _anjay_get_bytes_unlocked(anjay_unlocked_input_ctx_t *ctx,
                              size_t *out_bytes_read,
                              bool *out_message_finished,
                              void *out_buf,
                              size_t buf_size) {
    char *buf_ptr = (char *) out_buf;
    size_t buf_left = buf_size;
    while (true) {
        size_t tmp_bytes_read = 0;
        int retval = get_some_bytes(ctx, &tmp_bytes_read, out_message_finished,
                                    buf_ptr, buf_left);
        buf_ptr += tmp_bytes_read;
        buf_left -= tmp_bytes_read;
        if (retval || *out_message_finished || !buf_left) {
            *out_bytes_read = buf_size - buf_left;
            return retval;
        }
    }
}

int anjay_get_bytes(anjay_input_ctx_t *ctx,
                    size_t *out_bytes_read,
                    bool *out_message_finished,
                    void *out_buf,
                    size_t buf_size) {
    int retval = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    retval = _anjay_get_bytes_unlocked(_anjay_input_get_unlocked(ctx),
                                       out_bytes_read, out_message_finished,
                                       out_buf, buf_size);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return retval;
}

int _anjay_get_string_unlocked(anjay_unlocked_input_ctx_t *ctx,
                               char *out_buf,
                               size_t buf_size) {
    if (!ctx->vtable->string) {
        return -1;
    }
    if (buf_size == 0) {
        // At least terminating nullbyte must fit into the buffer!
        return ANJAY_BUFFER_TOO_SHORT;
    }
    return ctx->vtable->string(ctx, out_buf, buf_size);
}

int anjay_get_string(anjay_input_ctx_t *ctx, char *out_buf, size_t buf_size) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_string_unlocked(_anjay_input_get_unlocked(ctx), out_buf,
                                        buf_size);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_get_i32_unlocked(anjay_unlocked_input_ctx_t *ctx, int32_t *out) {
    int64_t tmp;
    int result = _anjay_get_i64_unlocked(ctx, &tmp);
    if (!result) {
        if (tmp < INT32_MIN || tmp > INT32_MAX) {
            result = ANJAY_ERR_BAD_REQUEST;
        } else {
            *out = (int32_t) tmp;
        }
    }
    return result;
}

int anjay_get_i32(anjay_input_ctx_t *ctx, int32_t *out) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_i32_unlocked(_anjay_input_get_unlocked(ctx), out);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_get_i64_unlocked(anjay_unlocked_input_ctx_t *ctx, int64_t *out) {
    if (!ctx->vtable->integer) {
        return -1;
    }
    return ctx->vtable->integer(ctx, out);
}

int anjay_get_i64(anjay_input_ctx_t *ctx, int64_t *out) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_i64_unlocked(_anjay_input_get_unlocked(ctx), out);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

#ifdef ANJAY_WITH_LWM2M11
int _anjay_get_u32_unlocked(anjay_unlocked_input_ctx_t *ctx, uint32_t *out) {
    uint64_t tmp;
    int result = _anjay_get_u64_unlocked(ctx, &tmp);
    if (!result) {
        if (tmp > UINT32_MAX) {
            result = ANJAY_ERR_BAD_REQUEST;
        } else {
            *out = (uint32_t) tmp;
        }
    }
    return result;
}

int anjay_get_u32(anjay_input_ctx_t *ctx, uint32_t *out) {
    int result = -1;
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_u32_unlocked(_anjay_input_get_unlocked(ctx), out);
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_get_u64_unlocked(anjay_unlocked_input_ctx_t *ctx, uint64_t *out) {
    if (!ctx->vtable->uint) {
        return -1;
    }
    return ctx->vtable->uint(ctx, out);
}

int anjay_get_u64(anjay_input_ctx_t *ctx, uint64_t *out) {
    int result = -1;
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_u64_unlocked(_anjay_input_get_unlocked(ctx), out);
#    ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#    endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}
#endif // ANJAY_WITH_LWM2M11

int anjay_get_float(anjay_input_ctx_t *ctx, float *out) {
    double tmp;
    int result = anjay_get_double(ctx, &tmp);
    if (!result) {
        *out = (float) tmp;
    }
    return result;
}

int _anjay_get_double_unlocked(anjay_unlocked_input_ctx_t *ctx, double *out) {
    if (!ctx->vtable->floating) {
        return -1;
    }
    return ctx->vtable->floating(ctx, out);
}

int anjay_get_double(anjay_input_ctx_t *ctx, double *out) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_double_unlocked(_anjay_input_get_unlocked(ctx), out);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_get_bool_unlocked(anjay_unlocked_input_ctx_t *ctx, bool *out) {
    if (!ctx->vtable->boolean) {
        return -1;
    }
    return ctx->vtable->boolean(ctx, out);
}

int anjay_get_bool(anjay_input_ctx_t *ctx, bool *out) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_bool_unlocked(_anjay_input_get_unlocked(ctx), out);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_get_objlnk_unlocked(anjay_unlocked_input_ctx_t *ctx,
                               anjay_oid_t *out_oid,
                               anjay_iid_t *out_iid) {
    if (!ctx->vtable->objlnk) {
        return -1;
    }
    return ctx->vtable->objlnk(ctx, out_oid, out_iid);
}

int anjay_get_objlnk(anjay_input_ctx_t *ctx,
                     anjay_oid_t *out_oid,
                     anjay_iid_t *out_iid) {
    int result = -1;
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_LOCK(anjay, ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    result = _anjay_get_objlnk_unlocked(_anjay_input_get_unlocked(ctx), out_oid,
                                        out_iid);
#ifdef ANJAY_WITH_THREAD_SAFETY
    ANJAY_MUTEX_UNLOCK(ctx->anjay_locked);
#endif // ANJAY_WITH_THREAD_SAFETY
    return result;
}

int _anjay_input_get_path(anjay_unlocked_input_ctx_t *ctx,
                          anjay_uri_path_t *out_path,
                          bool *out_is_array) {
    if (!ctx->vtable->get_path) {
        return ANJAY_ERR_BAD_REQUEST;
    }
    bool ignored_is_array;
    if (!out_is_array) {
        out_is_array = &ignored_is_array;
    }
    anjay_uri_path_t ignored_path;
    if (!out_path) {
        out_path = &ignored_path;
    }
    (void) ignored_is_array;
    (void) ignored_path;
    return ctx->vtable->get_path(ctx, out_path, out_is_array);
}

int _anjay_input_update_root_path(anjay_unlocked_input_ctx_t *ctx,
                                  const anjay_uri_path_t *root_path) {
    if (!ctx->vtable->update_root_path) {
        return ANJAY_ERR_BAD_REQUEST;
    }
    return ctx->vtable->update_root_path(ctx, root_path);
}

int _anjay_input_next_entry(anjay_unlocked_input_ctx_t *ctx) {
    if (!ctx->vtable->next_entry) {
        return -1;
    }
    return ctx->vtable->next_entry(ctx);
}

int _anjay_input_ctx_destroy(anjay_unlocked_input_ctx_t **ctx_ptr) {
    int retval = 0;
    anjay_unlocked_input_ctx_t *ctx = *ctx_ptr;
    if (ctx) {
        if (ctx->vtable->close) {
            retval = ctx->vtable->close(*ctx_ptr);
        }
        avs_free(ctx);
        *ctx_ptr = NULL;
    }
    return retval;
}

#ifdef ANJAY_TEST
#    include "tests/core/io.c"
#endif
