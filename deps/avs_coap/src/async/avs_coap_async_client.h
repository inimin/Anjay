/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem CoAP library
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#ifndef AVS_COAP_SRC_ASYNC_CLIENT_H
#define AVS_COAP_SRC_ASYNC_CLIENT_H

#include <avsystem/coap/async_client.h>

#include "avs_coap_ctx_vtable.h"

VISIBILITY_PRIVATE_HEADER_BEGIN

/**
 * Additional exchange data required by outgoing requests currently being
 * processed by us (acting as a CoAP client).
 */
typedef struct avs_coap_client_exchange_data {
    /**
     * User-defined handler to be called whenever a response to sent message
     * is received.
     *
     * Note: called by the async layer from within
     * @ref avs_coap_exchange_handlers_t#send_result_handler .
     */
    avs_coap_client_async_response_handler_t *handle_response;
    void *handle_response_arg;

    /**
     * Internal handler used by async layer to handle intermediate responses,
     * (e.g. 2.31 Continue).
     */
    avs_coap_send_result_handler_t *send_result_handler;
    void *send_result_handler_arg;

    /**
     * Used to update BLOCK2 option in requests for more response payload.
     * This is required because BERT may make the offset increment by more than
     * a single block size.
     */
    size_t next_response_payload_offset;

    /** ETag from the first response. */
    avs_coap_etag_t etag;
    /** Indicating that ETag from the first response was stored. */
    bool etag_stored;
} avs_coap_client_exchange_data_t;

struct avs_coap_exchange;

avs_error_t _avs_coap_client_exchange_send_first_chunk(
        avs_coap_ctx_t *ctx,
        AVS_LIST(struct avs_coap_exchange) **exchange_ptr_ptr);

bool _avs_coap_client_exchange_request_sent(
        const struct avs_coap_exchange *exchange);

/** Cleans up any resources associated with client-side @p exchange . */
void _avs_coap_client_exchange_cleanup(avs_coap_ctx_t *ctx,
                                       struct avs_coap_exchange *exchange,
                                       avs_error_t err);

VISIBILITY_PRIVATE_HEADER_END

#endif // AVS_COAP_SRC_ASYNC_CLIENT_H
