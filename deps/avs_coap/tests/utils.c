/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem CoAP library
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#include <avs_coap_init.h>

#ifdef AVS_UNIT_TESTING

#    include <avsystem/coap/coap.h>
#    include <avsystem/commons/avs_utils.h>

#    include "utils.h"

#    define MODULE_NAME test
#    include <avs_coap_x_log_config.h>

static uint64_t GLOBAL_TOKEN_VALUE;

void reset_token_generator(void) {
    GLOBAL_TOKEN_VALUE = 0;
}

avs_error_t _avs_coap_ctx_generate_token(avs_coap_ctx_t *ctx,
                                         avs_coap_token_t *out_token);

avs_error_t _avs_coap_ctx_generate_token(avs_coap_ctx_t *ctx,
                                         avs_coap_token_t *out_token) {
    (void) ctx;
    *out_token = nth_token(GLOBAL_TOKEN_VALUE++);
    return AVS_OK;
}

avs_coap_token_t nth_token(uint64_t k) {
    union {
        uint8_t bytes[sizeof(uint64_t)];
        uint64_t value;
    } v;
    v.value = avs_convert_be64(k);

    avs_coap_token_t token;
    token.size = sizeof(v.bytes);
    memcpy(token.bytes, v.bytes, sizeof(v.bytes));
    return token;
}

avs_coap_token_t current_token(void) {
    return nth_token(GLOBAL_TOKEN_VALUE);
}

#endif // AVS_UNIT_TESTING
