#include <assert.h>
#include <string.h>

#include <anjay/anjay.h>
#include <anjay/security.h>
#include <anjay/server.h>

#include <avsystem/commons/avs_log.h>

typedef struct test_instance {
    bool has_label;
    char label[32];

    bool has_value;
    int32_t value;
} test_instance_t;

static const test_instance_t DEFAULT_INSTANCE_VALUES[] = {
    { true, "First", true, 1 }, { true, "Second", true, 2 }
};
#define NUM_INSTANCES \
    (sizeof(DEFAULT_INSTANCE_VALUES) / sizeof(DEFAULT_INSTANCE_VALUES[0]))

typedef struct test_object {
    // handlers
    const anjay_dm_object_def_t *const obj_def;

    // object state
    test_instance_t instances[NUM_INSTANCES];

    test_instance_t backup_instances[NUM_INSTANCES];
} test_object_t;

static test_object_t *get_test_object(const anjay_dm_object_def_t *const *obj) {
    assert(obj);

    // use the container_of pattern to retrieve test_object_t pointer
    // AVS_CONTAINER_OF macro provided by avsystem/commons/defs.h
    return AVS_CONTAINER_OF(obj, test_object_t, obj_def);
}

static int test_list_instances(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *obj_ptr,
                               anjay_dm_list_ctx_t *ctx) {
    (void) anjay;   // unused
    (void) obj_ptr; // unused

    for (anjay_iid_t iid = 0; iid < NUM_INSTANCES; ++iid) {
        anjay_dm_emit(ctx, iid);
    }

    return 0;
}

static int test_instance_reset(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *obj_ptr,
                               anjay_iid_t iid) {
    (void) anjay; // unused

    test_object_t *test = get_test_object(obj_ptr);

    // IID validity was checked by the `anjay_dm_list_instances_t` handler.
    // If the Object Instance set does not change, or can only be modified
    // via LwM2M Create/Delete requests, it is safe to assume IID is correct.
    assert((size_t) iid < NUM_INSTANCES);

    // mark all Resource values for Object Instance `iid` as unset
    test->instances[iid].has_label = false;
    test->instances[iid].has_value = false;
    return 0;
}

static int test_list_resources(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *obj_ptr,
                               anjay_iid_t iid,
                               anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;   // unused
    (void) obj_ptr; // unused
    (void) iid;     // unused

    anjay_dm_emit_res(ctx, 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    return 0;
}

static int test_resource_read(anjay_t *anjay,
                              const anjay_dm_object_def_t *const *obj_ptr,
                              anjay_iid_t iid,
                              anjay_rid_t rid,
                              anjay_riid_t riid,
                              anjay_output_ctx_t *ctx) {
    (void) anjay; // unused

    test_object_t *test = get_test_object(obj_ptr);

    // IID validity was checked by the `anjay_dm_list_instances_t` handler.
    // If the Object Instance set does not change, or can only be modified
    // via LwM2M Create/Delete requests, it is safe to assume IID is correct.
    assert((size_t) iid < NUM_INSTANCES);
    const struct test_instance *current_instance = &test->instances[iid];

    // We have no Multiple-Instance Resources, so it is safe to assume
    // that RIID is never set.
    assert(riid == ANJAY_ID_INVALID);

    switch (rid) {
    case 0:
        return anjay_ret_string(ctx, current_instance->label);
    case 1:
        return anjay_ret_i32(ctx, current_instance->value);
    default:
        // control will never reach this part due to test_list_resources
        return ANJAY_ERR_INTERNAL;
    }
}

static int test_resource_write(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *obj_ptr,
                               anjay_iid_t iid,
                               anjay_rid_t rid,
                               anjay_riid_t riid,
                               anjay_input_ctx_t *ctx) {
    (void) anjay; // unused

    test_object_t *test = get_test_object(obj_ptr);

    // IID validity was checked by the `anjay_dm_list_instances_t` handler.
    // If the Object Instance set does not change, or can only be modified
    // via LwM2M Create/Delete requests, it is safe to assume IID is correct.
    assert((size_t) iid < NUM_INSTANCES);
    struct test_instance *current_instance = &test->instances[iid];

    // We have no Multiple-Instance Resources, so it is safe to assume
    // that RIID is never set.
    assert(riid == ANJAY_ID_INVALID);

    switch (rid) {
    case 0: {
        // `anjay_get_string` may return a chunk of data instead of the
        // whole value - we need to make sure the client is able to hold
        // the entire value
        char buffer[sizeof(current_instance->label)];
        int result = anjay_get_string(ctx, buffer, sizeof(buffer));

        if (result == 0) {
            // value OK - save it
            memcpy(current_instance->label, buffer, sizeof(buffer));
            current_instance->has_label = true;
        } else if (result == ANJAY_BUFFER_TOO_SHORT) {
            // the value is too long to store in the buffer
            result = ANJAY_ERR_BAD_REQUEST;
        }

        return result;
    }

    case 1: {
        // reading primitive values can be done directly - the value will only
        // be written to the output variable if everything went fine
        int result = anjay_get_i32(ctx, &current_instance->value);
        if (result == 0) {
            current_instance->has_value = true;
        }
        return result;
    }

    default:
        // control will never reach this part due to test_list_resources
        return ANJAY_ERR_INTERNAL;
    }
}

static int test_transaction_begin(anjay_t *anjay,
                                  const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay; // unused

    test_object_t *test = get_test_object(obj_ptr);

    // store a snapshot of object state
    memcpy(test->backup_instances, test->instances, sizeof(test->instances));
    return 0;
}

static int
test_transaction_validate(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay; // unused

    test_object_t *test = get_test_object(obj_ptr);

    // ensure all Object Instances contain all Mandatory Resources
    for (size_t i = 0; i < NUM_INSTANCES; ++i) {
        if (!test->instances[i].has_label || !test->instances[i].has_value) {
            // validation failed: Object state invalid, rollback required
            return ANJAY_ERR_BAD_REQUEST;
        }
    }

    // validation successful, can commit
    return 0;
}

static int
test_transaction_commit(anjay_t *anjay,
                        const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;   // unused
    (void) obj_ptr; // unused

    // no action required in this implementation; if object state snapshot was
    // dynamically allocated, this would be the place for releasing it
    return 0;
}

static int
test_transaction_rollback(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay; // unused

    test_object_t *test = get_test_object(obj_ptr);

    // restore saved object state
    memcpy(test->instances, test->backup_instances, sizeof(test->instances));
    return 0;
}

static const anjay_dm_object_def_t OBJECT_DEF = {
    // Object ID
    .oid = 1234,

    .handlers = {
        .list_instances = test_list_instances,
        .instance_reset = test_instance_reset,

        .list_resources = test_list_resources,
        .resource_read = test_resource_read,
        .resource_write = test_resource_write,

        .transaction_begin = test_transaction_begin,
        .transaction_validate = test_transaction_validate,
        .transaction_commit = test_transaction_commit,
        .transaction_rollback = test_transaction_rollback
    }
};

static int setup_security_object(anjay_t *anjay) {
    const anjay_security_instance_t security_instance = {
        .ssid = 1,
        .server_uri = "coap://try-anjay.avsystem.com:5683",
        .security_mode = ANJAY_SECURITY_NOSEC
    };

    if (anjay_security_object_install(anjay)) {
        return -1;
    }

    // let Anjay assign an Object Instance ID
    anjay_iid_t security_instance_id = ANJAY_ID_INVALID;
    if (anjay_security_object_add_instance(anjay, &security_instance,
                                           &security_instance_id)) {
        return -1;
    }

    return 0;
}

static int setup_server_object(anjay_t *anjay) {
    const anjay_server_instance_t server_instance = {
        .ssid = 1,
        .lifetime = 86400,
        .default_min_period = -1,
        .default_max_period = -1,
        .disable_timeout = -1,
        .binding = "U"
    };

    if (anjay_server_object_install(anjay)) {
        return -1;
    }

    anjay_iid_t server_instance_id = ANJAY_ID_INVALID;
    if (anjay_server_object_add_instance(anjay, &server_instance,
                                         &server_instance_id)) {
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        avs_log(tutorial, ERROR, "usage: %s ENDPOINT_NAME", argv[0]);
        return -1;
    }

    const anjay_configuration_t CONFIG = {
        .endpoint_name = argv[1],
        .in_buffer_size = 4000,
        .out_buffer_size = 4000
    };

    anjay_t *anjay = anjay_new(&CONFIG);
    if (!anjay) {
        return -1;
    }

    int result = 0;

    if (setup_security_object(anjay) || setup_server_object(anjay)) {
        result = -1;
        goto cleanup;
    }

    // initialize and register the test object
    test_object_t test_object = {
        .obj_def = &OBJECT_DEF
    };
    for (size_t i = 0; i < NUM_INSTANCES; ++i) {
        test_object.instances[i] = DEFAULT_INSTANCE_VALUES[i];
    }

    anjay_register_object(anjay, &test_object.obj_def);

    result = anjay_event_loop_run(anjay,
                                  avs_time_duration_from_scalar(1, AVS_TIME_S));

cleanup:
    anjay_delete(anjay);

    // test object does not need cleanup
    return result;
}
