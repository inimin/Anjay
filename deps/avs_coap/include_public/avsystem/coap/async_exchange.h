/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem CoAP library
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#ifndef AVSYSTEM_COAP_ASYNC_EXCHANGE_H
#define AVSYSTEM_COAP_ASYNC_EXCHANGE_H

#include <avsystem/coap/avs_coap_config.h>

#include <stdint.h>

#include <avsystem/coap/ctx.h>
#include <avsystem/coap/writer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An ID used to uniquely identify an asynchronous request within CoAP context.
 */
typedef struct {
    uint64_t value;
} avs_coap_exchange_id_t;

/**
 * Placeholder exchange ID that is guaranteed <em>not</em> to identify any
 * exchange existing at any point of time.
 */
static const avs_coap_exchange_id_t AVS_COAP_EXCHANGE_ID_INVALID = { 0 };

static inline bool avs_coap_exchange_id_equal(avs_coap_exchange_id_t a,
                                              avs_coap_exchange_id_t b) {
    return a.value == b.value;
}

static inline bool avs_coap_exchange_id_valid(avs_coap_exchange_id_t id) {
    return !avs_coap_exchange_id_equal(id, AVS_COAP_EXCHANGE_ID_INVALID);
}

/**
 * Releases all memory associated with not-yet-delivered request.
 * If the exchange is a request and <c>response_handler</c> has been set to
 * non-NULL when creating it, it is called with
 * @ref AVS_COAP_CLIENT_REQUEST_CANCEL .
 *
 * @param ctx         CoAP context to operate on.
 *
 * @param exchange_id ID of the undelivered request that should be canceled.
 *                    If the request was already delivered or represents a
 *                    request not known by the @p ctx, nothing happens.
 */
void avs_coap_exchange_cancel(avs_coap_ctx_t *ctx,
                              avs_coap_exchange_id_t exchange_id);

#ifdef __cplusplus
}
#endif

#endif // AVSYSTEM_COAP_ASYNC_EXCHANGE_H
