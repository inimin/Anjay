/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem Anjay LwM2M SDK
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#if !defined(_POSIX_C_SOURCE) && !defined(__APPLE__)
#    define _POSIX_C_SOURCE 200809L
#endif

#include "../demo.h"
#include "../demo_utils.h"
#include "../objects.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>

#include <avsystem/commons/avs_utils.h>

#define IP_PING_HOSTNAME 0
#define IP_PING_REPETITIONS 1
#define IP_PING_TIMEOUT_MS 2
#define IP_PING_BLOCK_SIZE 3
#define IP_PING_DSCP 4
#define IP_PING_RUN 5
#define IP_PING_STATE 6
#define IP_PING_SUCCESS_COUNT 7
#define IP_PING_ERROR_COUNT 8
#define IP_PING_AVG_TIME_MS 9
#define IP_PING_MIN_TIME_MS 10
#define IP_PING_MAX_TIME_MS 11
#define IP_PING_TIME_STDEV_US 12

typedef enum {
    IP_PING_STATE_NONE,
    IP_PING_STATE_IN_PROGRESS,
    IP_PING_STATE_COMPLETE,
    IP_PING_STATE_ERROR_HOST_NAME,
    IP_PING_STATE_ERROR_INTERNAL,
    IP_PING_STATE_ERROR_OTHER
} ip_ping_state_t;

typedef struct {
    char hostname[257];
    uint32_t repetitions;
    uint32_t ms_timeout;
    uint16_t block_size;
    uint8_t dscp;
} ip_ping_conf_t;

typedef struct {
    volatile atomic_int state_;
    volatile atomic_uint success_count_;
    volatile atomic_uint error_count_;
    volatile atomic_uint avg_response_time_;
    volatile atomic_uint min_response_time_;
    volatile atomic_uint max_response_time_;
    volatile atomic_uint response_time_stdev_us_;
} ip_ping_stats_t;

typedef struct {
    FILE *ping_pipe;
    anjay_t *anjay;
} ip_ping_command_state_t;

typedef struct {
    const anjay_dm_object_def_t *def;
    ip_ping_conf_t configuration;
    ip_ping_conf_t saved_configuration;
    ip_ping_stats_t stats;
    ip_ping_command_state_t command_state;
} ip_ping_t;

static inline ip_ping_t *
get_ip_ping(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, ip_ping_t, def);
}

static int ip_ping_list_resources(anjay_t *anjay,
                                  const anjay_dm_object_def_t *const *obj_ptr,
                                  anjay_iid_t iid,
                                  anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, IP_PING_HOSTNAME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_REPETITIONS, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_TIMEOUT_MS, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_BLOCK_SIZE, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_DSCP, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_RUN, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_STATE, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_SUCCESS_COUNT, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_ERROR_COUNT, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_AVG_TIME_MS, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_MIN_TIME_MS, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_MAX_TIME_MS, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, IP_PING_TIME_STDEV_US, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    return 0;
}

static int ip_ping_resource_read(anjay_t *anjay,
                                 const anjay_dm_object_def_t *const *obj_ptr,
                                 anjay_iid_t iid,
                                 anjay_rid_t rid,
                                 anjay_riid_t riid,
                                 anjay_output_ctx_t *ctx) {
    (void) anjay;
    (void) iid;
    (void) riid;
    assert(riid == ANJAY_ID_INVALID);
    ip_ping_t *ping = get_ip_ping(obj_ptr);

    switch (rid) {
    case IP_PING_HOSTNAME:
        return anjay_ret_string(ctx, ping->configuration.hostname);
    case IP_PING_REPETITIONS:
        return anjay_ret_i64(ctx, ping->configuration.repetitions);
    case IP_PING_TIMEOUT_MS:
        return anjay_ret_i64(ctx, ping->configuration.ms_timeout);
    case IP_PING_BLOCK_SIZE:
        return anjay_ret_i32(ctx, ping->configuration.block_size);
    case IP_PING_DSCP:
        return anjay_ret_i32(ctx, ping->configuration.dscp);
    case IP_PING_STATE:
        return anjay_ret_i32(ctx, (int32_t) atomic_load(&ping->stats.state_));
    case IP_PING_SUCCESS_COUNT:
        return anjay_ret_i64(ctx, atomic_load(&ping->stats.success_count_));
    case IP_PING_ERROR_COUNT:
        return anjay_ret_i64(ctx, atomic_load(&ping->stats.error_count_));
    case IP_PING_AVG_TIME_MS:
        return anjay_ret_i64(ctx, atomic_load(&ping->stats.avg_response_time_));
    case IP_PING_MIN_TIME_MS:
        return anjay_ret_i64(ctx, atomic_load(&ping->stats.min_response_time_));
    case IP_PING_MAX_TIME_MS:
        return anjay_ret_i64(ctx, atomic_load(&ping->stats.max_response_time_));
    case IP_PING_TIME_STDEV_US:
        return anjay_ret_i64(ctx,
                             atomic_load(&ping->stats.response_time_stdev_us_));
    default:
        AVS_UNREACHABLE(
                "Read handler called on unknown or non-readable resource");
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int ip_ping_reset_diagnostic_state(anjay_t *anjay, ip_ping_t *ipping) {
    int state = IP_PING_STATE_COMPLETE;
    bool reset = false;
    while (!(reset = atomic_compare_exchange_weak(&ipping->stats.state_, &state,
                                                  IP_PING_STATE_NONE))
           && state != IP_PING_STATE_NONE && state != IP_PING_STATE_IN_PROGRESS)
        ;
    if (reset) {
        anjay_notify_changed(anjay, ipping->def->oid, 0, IP_PING_STATE);
    } else if (state == IP_PING_STATE_IN_PROGRESS) {
        demo_log(ERROR, "Canceling a diagnostic in progress is not supported");
        return ANJAY_ERR_INTERNAL;
    }
    return 0;
}

#define DECLARE_GET_NUM(Type, Base)                                            \
    static int get_##Type(anjay_input_ctx_t *ctx, Type##_t *out, Type##_t min, \
                          Type##_t max) {                                      \
        int##Base##_t base;                                                    \
        int result = anjay_get_i##Base(ctx, &base);                            \
        if (result) {                                                          \
            return result;                                                     \
        }                                                                      \
        if (base < (int##Base##_t) min || base > (int##Base##_t) max) {        \
            return ANJAY_ERR_BAD_REQUEST;                                      \
        }                                                                      \
        *out = (Type##_t) base;                                                \
        return 0;                                                              \
    }

DECLARE_GET_NUM(uint8, 32)
DECLARE_GET_NUM(uint16, 32)
DECLARE_GET_NUM(uint32, 64)

static int ip_ping_resource_write(anjay_t *anjay,
                                  const anjay_dm_object_def_t *const *obj_ptr,
                                  anjay_iid_t iid,
                                  anjay_rid_t rid,
                                  anjay_riid_t riid,
                                  anjay_input_ctx_t *ctx) {
    (void) anjay;
    (void) iid;
    (void) riid;
    assert(riid == ANJAY_ID_INVALID);
    ip_ping_t *ping = get_ip_ping(obj_ptr);
    int result;

    switch (rid) {
    case IP_PING_HOSTNAME:
        (void) ((result = ip_ping_reset_diagnostic_state(anjay, ping))
                || (result = anjay_get_string(
                            ctx, ping->configuration.hostname,
                            sizeof(ping->configuration.hostname))));
        return result;
    case IP_PING_REPETITIONS:
        (void) ((result = ip_ping_reset_diagnostic_state(anjay, ping))
                || (result = get_uint32(ctx, &ping->configuration.repetitions,
                                        1, UINT32_MAX)));
        return result;
    case IP_PING_TIMEOUT_MS:
        (void) ((result = ip_ping_reset_diagnostic_state(anjay, ping))
                || (result = get_uint32(ctx, &ping->configuration.ms_timeout, 1,
                                        UINT32_MAX)));
        return result;
    case IP_PING_BLOCK_SIZE:
        (void) ((result = ip_ping_reset_diagnostic_state(anjay, ping))
                || (result = get_uint16(ctx, &ping->configuration.block_size, 1,
                                        UINT16_MAX)));
        return result;
    case IP_PING_DSCP:
        (void) ((result = ip_ping_reset_diagnostic_state(anjay, ping))
                || (result = get_uint8(ctx, &ping->configuration.dscp, 0, 63)));
        return result;
    default:
        // Bootstrap Server may try to write to other resources,
        // so no AVS_UNREACHABLE() here
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static void update_response_times(ip_ping_t *ping,
                                  unsigned min,
                                  unsigned max,
                                  unsigned avg,
                                  unsigned mdev_us) {
    atomic_store(&ping->stats.min_response_time_, min);
    anjay_notify_changed(ping->command_state.anjay, ping->def->oid, 0,
                         IP_PING_MIN_TIME_MS);
    atomic_store(&ping->stats.avg_response_time_, max);
    anjay_notify_changed(ping->command_state.anjay, ping->def->oid, 0,
                         IP_PING_AVG_TIME_MS);
    atomic_store(&ping->stats.max_response_time_, avg);
    anjay_notify_changed(ping->command_state.anjay, ping->def->oid, 0,
                         IP_PING_MAX_TIME_MS);
    atomic_store(&ping->stats.response_time_stdev_us_, mdev_us);
    anjay_notify_changed(ping->command_state.anjay, ping->def->oid, 0,
                         IP_PING_TIME_STDEV_US);
}

static void ip_ping_command_state_cleanup(ip_ping_t *ping) {
    if (ping->command_state.ping_pipe) {
        pclose(ping->command_state.ping_pipe);
        ping->command_state.ping_pipe = NULL;
    }
}

static void *ip_ping_thread(void *ping_) {
    ip_ping_t *ping = (ip_ping_t *) ping_;
    char line[512];

    typedef enum {
        IP_PING_HANDLER_HEADER,
        IP_PING_HANDLER_SKIP1,
        IP_PING_HANDLER_SKIP2,
        IP_PING_HANDLER_COUNTS,
        IP_PING_HANDLER_RTT
    } ip_ping_handler_state_t;

    ip_ping_handler_state_t state = IP_PING_HANDLER_HEADER;

    while (fgets(line, sizeof(line), ping->command_state.ping_pipe)) {
        switch (state) {
        case IP_PING_HANDLER_HEADER:
            if (strstr(line, "unknown")) {
                demo_log(ERROR, "Unknown host: %s",
                         ping->configuration.hostname);
                atomic_store(&ping->stats.state_,
                             IP_PING_STATE_ERROR_HOST_NAME);
                goto finish;
            }
            break;
        case IP_PING_HANDLER_SKIP1:
        case IP_PING_HANDLER_SKIP2:
        default:
            break;
        case IP_PING_HANDLER_COUNTS: {
            unsigned total, success;
            if (sscanf(line, "%u %*s %*s %u", &total, &success) != 2) {
                goto finish;
            }
            atomic_store(&ping->stats.success_count_, success);
            anjay_notify_changed(ping->command_state.anjay, ping->def->oid, 0,
                                 IP_PING_SUCCESS_COUNT);
            atomic_store(&ping->stats.error_count_, total - success);
            anjay_notify_changed(ping->command_state.anjay, ping->def->oid, 0,
                                 IP_PING_ERROR_COUNT);
            if (success == 0) {
                atomic_store(&ping->stats.state_, IP_PING_STATE_COMPLETE);
                update_response_times(ping, 0, 0, 0, 0);
                goto finish;
            }
            break;
        }
        case IP_PING_HANDLER_RTT: {
            const char *ptr = strstr(line, "=");
            if (!ptr) {
                demo_log(ERROR, "Invalid output format of ping.");
                goto finish;
            }
            ptr += 2;
            float min, avg, max, mdev;
            if (sscanf(ptr, "%f/%f/%f/%f", &min, &avg, &max, &mdev) != 4) {
                goto finish;
            }
            atomic_store(&ping->stats.state_, IP_PING_STATE_COMPLETE);
            update_response_times(ping,
                                  (unsigned) min,
                                  (unsigned) avg,
                                  (unsigned) max,
                                  (unsigned) (mdev * 1000.0f));
        }
        }
        state = (ip_ping_handler_state_t) (state + 1);
    }

finish:
    ip_ping_command_state_cleanup(ping);
    atomic_compare_exchange_strong(
            &ping->stats.state_, &(unsigned int) { IP_PING_STATE_IN_PROGRESS },
            IP_PING_STATE_ERROR_INTERNAL);
    anjay_notify_changed(ping->command_state.anjay, ping->def->oid, 0,
                         IP_PING_STATE);
    return NULL;
}

static ip_ping_state_t start_ip_ping(anjay_t *anjay, ip_ping_t *ping) {
    if (!ping->configuration.repetitions || !ping->configuration.ms_timeout
            || !ping->configuration.block_size
            || !ping->configuration.hostname[0]) {
        return IP_PING_STATE_ERROR_OTHER;
    }

    char command[320];
    unsigned timeout_s = ping->configuration.ms_timeout / 1000;
    if (!timeout_s) {
        timeout_s = 1;
    }

    if (avs_simple_snprintf(command, sizeof(command),
                            "ping -q -c %u -Q 0x%x -W %u -s %u %s 2>&1",
                            ping->configuration.repetitions,
                            ping->configuration.dscp << 2, timeout_s,
                            ping->configuration.block_size,
                            ping->configuration.hostname)
            < 0) {
        demo_log(ERROR, "Cannot prepare ping command");
        return IP_PING_STATE_ERROR_INTERNAL;
    }

    if (!(ping->command_state.ping_pipe = popen(command, "r"))) {
        demo_log(ERROR, "Cannot start child process. Command: %s", command);
        return IP_PING_STATE_ERROR_INTERNAL;
    }

    ping->command_state.anjay = anjay;
    pthread_t thread;
    if (pthread_create(&thread, NULL, ip_ping_thread, NULL)) {
        pclose(ping->command_state.ping_pipe);
        ping->command_state.ping_pipe = NULL;
        return IP_PING_STATE_ERROR_INTERNAL;
    }

    pthread_detach(thread);
    return IP_PING_STATE_IN_PROGRESS;
}

static int ip_ping_resource_execute(anjay_t *anjay,
                                    const anjay_dm_object_def_t *const *obj_ptr,
                                    anjay_iid_t iid,
                                    anjay_rid_t rid,
                                    anjay_execute_ctx_t *arg_ctx) {
    (void) arg_ctx;
    (void) rid;
    ip_ping_t *ping = get_ip_ping(obj_ptr);
    int result;

    assert(rid == IP_PING_RUN);
    if ((result = ip_ping_reset_diagnostic_state(anjay, ping))) {
        return result;
    }
    ip_ping_state_t state = start_ip_ping(anjay, ping);
    if (atomic_compare_exchange_strong(&ping->stats.state_,
                                       &(int) { IP_PING_STATE_NONE }, state)) {
        anjay_notify_changed(anjay, (*obj_ptr)->oid, iid, IP_PING_STATE);
    }
    return 0;
}

static int
ip_ping_transaction_begin(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;
    ip_ping_t *repr = get_ip_ping(obj_ptr);
    repr->saved_configuration = repr->configuration;
    return 0;
}

static int
ip_ping_transaction_rollback(anjay_t *anjay,
                             const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;
    ip_ping_t *repr = get_ip_ping(obj_ptr);
    repr->configuration = repr->saved_configuration;
    return 0;
}

static const anjay_dm_object_def_t IP_PING = {
    .oid = DEMO_OID_IP_PING,
    .handlers = {
        .list_instances = anjay_dm_list_instances_SINGLE,
        .list_resources = ip_ping_list_resources,
        .resource_read = ip_ping_resource_read,
        .resource_write = ip_ping_resource_write,
        .resource_execute = ip_ping_resource_execute,
        .transaction_begin = ip_ping_transaction_begin,
        .transaction_validate = anjay_dm_transaction_NOOP,
        .transaction_commit = anjay_dm_transaction_NOOP,
        .transaction_rollback = ip_ping_transaction_rollback
    }
};

const anjay_dm_object_def_t **ip_ping_object_create(void) {
    ip_ping_t *repr = (ip_ping_t *) avs_calloc(1, sizeof(ip_ping_t));
    if (!repr) {
        return NULL;
    }

    repr->def = &IP_PING;

    return &repr->def;
}

void ip_ping_object_release(const anjay_dm_object_def_t **def) {
    if (def) {
        ip_ping_t *ping = get_ip_ping(def);
        ip_ping_command_state_cleanup(ping);
        avs_free(ping);
    }
}
