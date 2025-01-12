/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem CoAP library
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#ifndef AVSYSTEM_COAP_ASYNC_H
#define AVSYSTEM_COAP_ASYNC_H

#include <avsystem/coap/avs_coap_config.h>

#include <stdint.h>

#include <avsystem/coap/async_client.h>
#include <avsystem/coap/async_exchange.h>
#include <avsystem/coap/async_server.h>
#include <avsystem/coap/ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Receives as much data from the socket associated with @p ctx as possible to
 * receive in a non-blocking way, and handles it as appropriate.
 *
 * [TCP] If received data is not a complete CoAP message or if it doesn't
 * contain at least a part of payload, this function does nothing except of
 * buffering these data internally. It is considered as successfull call if
 * there is enough space in the buffer.
 *
 * If the packet is recognized as part of a known ongoing exchange, such
 * message is handled internally without calling @p handle_request . Otherwise,
 * incoming message is passed to @p handle_request .
 *
 * This function should be called every time when user detect new data arrived
 * on the socket assigned to @p ctx .
 *
 * When this function calls the receive method on the socket, the receive
 * timeout is always set to zero. If you wish to perform a blocking receive
 * operation, please use <c>poll()</c> or a similar system API first.
 *
 * @param ctx                     CoAP context associated with the socket to
 *                                receive the message from.
 *
 * @param[in]  handle_request     Callback used to handle incoming requests. May
 *                                be NULL, in which case it will only handle
 *                                responses to asynchronous requests and ignore
 *                                incoming requests.
 *
 * @param[in]  handle_request_arg An opaque argument passed to
 *                                @p handle_request .
 *
 * @returns <c>AVS_OK</c> for success, or an error condition for which the
 *          operation failed.
 */
avs_error_t avs_coap_async_handle_incoming_packet(
        avs_coap_ctx_t *ctx,
        avs_coap_server_new_async_request_handler_t *handle_request,
        void *handle_request_arg);

#ifdef __cplusplus
}
#endif

#endif // AVSYSTEM_COAP_ASYNC_H
