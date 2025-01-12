/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem Anjay LwM2M SDK
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#include <anjay_init.h>

#include <math.h>

#define AVS_UNIT_ENABLE_SHORT_ASSERTS
#include <avsystem/commons/avs_unit_mocksock.h>
#include <avsystem/commons/avs_unit_test.h>

#include "src/core/anjay_core.h"
#include "src/core/io/anjay_vtable.h"
#include "src/core/servers/anjay_servers_internal.h"
#include "tests/core/coap/utils.h"
#include "tests/utils/dm.h"

AVS_UNIT_TEST(debug, debug_make_path_macro) {
    anjay_request_t request;
    request.uri = MAKE_ROOT_PATH();
    AVS_UNIT_ASSERT_EQUAL_STRING(ANJAY_DEBUG_MAKE_PATH(&request.uri), "/");
    request.uri = MAKE_OBJECT_PATH(0);
    AVS_UNIT_ASSERT_EQUAL_STRING(ANJAY_DEBUG_MAKE_PATH(&request.uri), "/0");
    request.uri = MAKE_INSTANCE_PATH(0, 1);
    AVS_UNIT_ASSERT_EQUAL_STRING(ANJAY_DEBUG_MAKE_PATH(&request.uri), "/0/1");
    request.uri = MAKE_RESOURCE_PATH(0, 1, 2);
    AVS_UNIT_ASSERT_EQUAL_STRING(ANJAY_DEBUG_MAKE_PATH(&request.uri), "/0/1/2");

    request.uri = MAKE_RESOURCE_PATH(65534, 65534, 65534);
    AVS_UNIT_ASSERT_EQUAL_STRING(ANJAY_DEBUG_MAKE_PATH(&request.uri),
                                 "/65534/65534/65534");
}

#ifdef ANJAY_WITH_LWM2M11
AVS_UNIT_TEST(dm_read, resource_instance) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 53, 64, 0,
            (const anjay_riid_t[]) { 75, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 53, 64, 75, 0,
                                        ANJAY_MOCK_DM_INT(0, 514));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(PLAINTEXT), PAYLOAD("514"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, resource_instance_read_err_concrete) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 53, 64, 0,
            (const anjay_riid_t[]) { 75, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 53, 64, 75,
                                        ANJAY_ERR_UNAUTHORIZED,
                                        ANJAY_MOCK_DM_NONE);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, UNAUTHORIZED, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, resource_instance_read_err_generic) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 53, 64, 0,
            (const anjay_riid_t[]) { 75, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 53, 64, 75, -1,
                                        ANJAY_MOCK_DM_NONE);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, resource_instance_not_found_because_not_present) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    // Empty multiple resource
    _anjay_mock_dm_expect_list_resource_instances(anjay, &OBJ, 53, 64, 0,
                                                  (const anjay_riid_t[]) {
                                                          ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}
#endif // ANJAY_WITH_LWM2M11

AVS_UNIT_TEST(dm_read, resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 514));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(PLAINTEXT), PAYLOAD("514"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, resource_read_err_concrete) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID,
                                        ANJAY_ERR_UNAUTHORIZED,
                                        ANJAY_MOCK_DM_NONE);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, UNAUTHORIZED, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, resource_read_err_generic) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID,
                                        -1, ANJAY_MOCK_DM_NONE);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, resource_not_found_because_not_present) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, instance_empty) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "13"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 13, 14, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 13, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(OMA_LWM2M_TLV), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, instance_some) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "13"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 13, 14, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 13, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 13, 0, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 69));
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 13, 6, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_STRING(0, "Hello"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(OMA_LWM2M_TLV),
                            PAYLOAD("\xc1\x00\x45"
                                    "\xc5\x06"
                                    "Hello"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, instance_resource_not_found) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "13"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 13, 14, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 13, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 13, 0, ANJAY_ID_INVALID,
                                        ANJAY_ERR_NOT_FOUND,
                                        ANJAY_MOCK_DM_NONE);
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 13, 1, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 69));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(OMA_LWM2M_TLV),
                            PAYLOAD("\xc1\x01\x45"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, instance_not_found) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "13"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 4, 14, 69, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, instance_err_concrete) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "13"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(anjay, &OBJ, ANJAY_ERR_UNAUTHORIZED,
                                         (const anjay_iid_t[]) {
                                                 ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, UNAUTHORIZED, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, instance_err_generic) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "13"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, -1, (const anjay_iid_t[]) { ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, object_empty) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(OMA_LWM2M_TLV), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, object_not_found) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("3"), NO_PAYLOAD);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, object_some) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 3, 7, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 3, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 7, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(OMA_LWM2M_TLV),
                            PAYLOAD("\x00\x03\x00\x07"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, object_err_concrete) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(anjay, &OBJ, ANJAY_ERR_UNAUTHORIZED,
                                         (const anjay_iid_t[]) {
                                                 ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, UNAUTHORIZED, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, object_err_generic) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42"), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, -1, (const anjay_iid_t[]) { ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, no_object) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), NO_PAYLOAD);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_REQUEST, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read, query) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    QUERY("depth=1"), NO_PAYLOAD);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_OPTION, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, force_tlv) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x2d16), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 514));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(OMA_LWM2M_TLV),
                            PAYLOAD("\xc2\x04\x02\x02"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, force_text_ok) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 514));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(PLAINTEXT), PAYLOAD("514"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, force_text_on_bytes) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_BYTES(0, "bytes"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(PLAINTEXT), PAYLOAD("Ynl0ZXM="));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, force_text_invalid) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69"),
                    ACCEPT(0), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_ACCEPTABLE, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, force_opaque_ok) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x2a), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_BYTES(0, "bytes"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(OCTET_STREAM), PAYLOAD("bytes"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, force_opaque_mismatch) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x2a), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(
            anjay, &OBJ, 69, 4, ANJAY_ID_INVALID, -1,
            ANJAY_MOCK_DM_INT(ANJAY_OUTCTXERR_METHOD_NOT_IMPLEMENTED, 514));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_ACCEPTABLE, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, force_opaque_invalid) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69"),
                    ACCEPT(0x2a), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_ACCEPTABLE, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_read_accept, invalid_format) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x4242), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 4, ANJAY_DM_RES_WM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_ACCEPTABLE, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

#ifdef ANJAY_WITH_LWM2M11
AVS_UNIT_TEST(dm_write, resource_instance) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), CONTENT_FORMAT(PLAINTEXT),
                    PAYLOAD("Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_WM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 53, 64, 0,
            (const anjay_riid_t[]) { 75, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 53, 64, 75,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_instance_resource_absent) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), CONTENT_FORMAT(PLAINTEXT),
                    PAYLOAD("Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_WM,
                                                    ANJAY_DM_RES_ABSENT },
                                                  ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_instance_resource_not_writable) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), CONTENT_FORMAT(PLAINTEXT),
                    PAYLOAD("Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_instance_resource_not_multiple) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), CONTENT_FORMAT(PLAINTEXT),
                    PAYLOAD("Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_W,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_instance_absent) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), CONTENT_FORMAT(PLAINTEXT),
                    PAYLOAD("Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_WM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    // Empty multiple resource
    _anjay_mock_dm_expect_list_resource_instances(anjay, &OBJ, 53, 64, 0,
                                                  (const anjay_riid_t[]) {
                                                          ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_instance_coap_format_tlv) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), CONTENT_FORMAT(OMA_LWM2M_TLV),
                    // [ 0x45 ][ 0x4B ]
                    // 01.............. Resource Instance
                    // ..0............. Identifier field is 8 bits long
                    // ...00........... No length field
                    // .....101........ Length = 5
                    // ........01001011 ID = 75
                    PAYLOAD("\x45\x4BHello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_WM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 53, 64, 0,
            (const anjay_riid_t[]) { 75, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 53, 64, 75,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_instance_coap_format_senml_cbor) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E),
                    PATH("42", "53", "64", "75"), CONTENT_FORMAT(SENML_CBOR),
                    // [{0 (CBOR_SENML_LABEL_NAME): "/42/53/64/75",
                    //   3 (CBOR_SENML_LABEL_VALUE_STRING): "Hello"}]
                    PAYLOAD("\x81\xA2\x00\x6C/42/53/64/75\x03\x65Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 53, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 53, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 64, ANJAY_DM_RES_WM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 53, 64, 0,
            (const anjay_riid_t[]) { 75, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 53, 64, 75,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}
#endif // ANJAY_WITH_LWM2M11

AVS_UNIT_TEST(dm_write, resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "514", "4"),
                    CONTENT_FORMAT(PLAINTEXT), PAYLOAD("Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 514, 4, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_plaintext_integer_with_leading_zero) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "514", "4"),
                    CONTENT_FORMAT(PLAINTEXT), PAYLOAD("0101"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 514, 4, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 101), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_unsupported_format) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "514", "4"),
                    CONTENT_FORMAT_VALUE(0x4242), PAYLOAD("Hello"));
    // 4.15 Unsupported Content Format.
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, UNSUPPORTED_CONTENT_FORMAT,
                            ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, resource_with_mismatched_tlv_rid) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "514", "4"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\xc5\x05"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_REQUEST, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "69"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 69, 0, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 13), 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 69, 6, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_unsupported_format) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "69"),
                    CONTENT_FORMAT_VALUE(0x4242),
                    PAYLOAD("\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    // 4.15 Unsupported Content Format
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, UNSUPPORTED_CONTENT_FORMAT,
                            ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_partial) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42", "69"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 69, 0, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 13), 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 69, 6, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_full) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("25", "69"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ_WITH_RESET, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_reset(anjay, &OBJ_WITH_RESET, 69, 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ_WITH_RESET, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ_WITH_RESET, 69, 0,
                                         ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 13), 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ_WITH_RESET, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ_WITH_RESET, 69, 6,
                                         ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_superfluous_instance) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("25", "69"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x08\x45\x0a"
                            "\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ_WITH_RESET, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_reset(anjay, &OBJ_WITH_RESET, 69, 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ_WITH_RESET, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ_WITH_RESET, 69, 0,
                                         ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 13), 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ_WITH_RESET, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ_WITH_RESET, 69, 6,
                                         ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_superfluous_and_empty) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("25", "1"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV), PAYLOAD("\x00\x01"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ_WITH_RESET, 0,
            (const anjay_iid_t[]) { 1, 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_reset(anjay, &OBJ_WITH_RESET, 1, 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_inconsistent_instance) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "69"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x08\x4d\x0a"
                            "\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_REQUEST, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_wrong_type) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "69"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x01\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_REQUEST, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, instance_nonexistent) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "69"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 4, 14, 514, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write, no_instance) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x08\x45\x0a"
                            "\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

#ifdef ANJAY_WITH_LWM2M11
AVS_UNIT_TEST(dm_write_composite, write_to_resource_of_nonexistent_instance) {
    DM_TEST_INIT;
    static const char PAYLOAD[] = "\x81\xa2\x00\x67"
                                  "/42/1/2"
                                  "\x02\x18\x2a";
    DM_TEST_REQUEST(mocksocks[0], CON, IPATCH, ID(0xFA3E),
                    CONTENT_FORMAT(SENML_CBOR),
                    PAYLOAD_EXTERNAL(PAYLOAD, sizeof(PAYLOAD) - 1));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 4, 14, 514, ANJAY_ID_INVALID });

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}
#endif // ANJAY_WITH_LWM2M11

AVS_UNIT_TEST(dm_execute, success) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("42", "514", "4"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_execute(anjay, &OBJ, 514, 4, NULL, 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_execute, data) {
    DM_TEST_INIT;
#define NYANCAT "Nyanyanyanyanyanyanya!"
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42", "514", "4"),
                    PAYLOAD("7='" NYANCAT "'"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_execute(
            anjay, &OBJ, 514, 4,
            ANJAY_MOCK_DM_EXECUTE(ANJAY_MOCK_DM_EXECUTE_ARG(0, 7, NYANCAT)), 0);
#undef NYANCAT
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_execute, error) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("42", "514", "4"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_execute(anjay, &OBJ, 514, 4, NULL,
                                           ANJAY_ERR_INTERNAL);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_execute, resource_inexistent) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("42", "514", "1"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_execute, instance_inexistent) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("42", "666", "1"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

static int
execute_get_arg_value_invalid_args(anjay_t *anjay,
                                   const anjay_dm_object_def_t *const *obj_ptr,
                                   anjay_iid_t iid,
                                   anjay_rid_t rid,
                                   anjay_execute_ctx_t *ctx) {
    (void) iid;
    (void) rid;
    (void) anjay;
    (void) obj_ptr;
    int ret;
    int arg;
    bool has_value;

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 0);
    AVS_UNIT_ASSERT_EQUAL(has_value, true);

    char buf[32];
    // buf_size < 2
    AVS_UNIT_ASSERT_FAILED(anjay_execute_get_arg_value(ctx, NULL, buf, 1));

    // buf == NULL
    AVS_UNIT_ASSERT_FAILED(anjay_execute_get_arg_value(ctx, NULL, NULL, 974));
    return 0;
}

AVS_UNIT_TEST(dm_execute, execute_get_arg_value_invalid_args) {
    DM_TEST_INIT;
    EXECUTE_OBJ->handlers.resource_execute = execute_get_arg_value_invalid_args;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("128", "514", "1"), PAYLOAD("0='foobarbaz'"));

    _anjay_mock_dm_expect_list_instances(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

static int valid_args_execute(anjay_t *anjay,
                              const anjay_dm_object_def_t *const *obj_ptr,
                              anjay_iid_t iid,
                              anjay_rid_t rid,
                              anjay_execute_ctx_t *ctx) {
    (void) iid;
    (void) rid;
    (void) anjay;
    (void) obj_ptr;
    int ret;
    int arg;
    bool has_value;

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 0);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 1);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 2);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 1);
    AVS_UNIT_ASSERT_EQUAL(arg, -1);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);
    return 0;
}

AVS_UNIT_TEST(dm_execute, valid_args) {
    DM_TEST_INIT;
    EXECUTE_OBJ->handlers.resource_execute = valid_args_execute;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("128", "514", "1"), PAYLOAD("0,1,2"));

    _anjay_mock_dm_expect_list_instances(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

static int
valid_args_with_values_execute(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *obj_ptr,
                               anjay_iid_t iid,
                               anjay_rid_t rid,
                               anjay_execute_ctx_t *ctx) {
    (void) iid;
    (void) rid;
    (void) anjay;
    (void) obj_ptr;
    int ret;
    int arg;
    bool has_value;

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 0);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 1);
    AVS_UNIT_ASSERT_EQUAL(has_value, true);

    char buf[32];
    size_t read_bytes;
    AVS_UNIT_ASSERT_SUCCESS(
            anjay_execute_get_arg_value(ctx, &read_bytes, buf, 32));
    AVS_UNIT_ASSERT_EQUAL(read_bytes, strlen("value"));
    AVS_UNIT_ASSERT_EQUAL_STRING(buf, "value");
    /* Already read everything. */
    AVS_UNIT_ASSERT_SUCCESS(
            anjay_execute_get_arg_value(ctx, &read_bytes, buf, 32));
    AVS_UNIT_ASSERT_EQUAL(read_bytes, 0);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 2);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 1);
    AVS_UNIT_ASSERT_EQUAL(arg, -1);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);
    return 0;
}

AVS_UNIT_TEST(dm_execute, valid_args_with_values) {
    DM_TEST_INIT;
    EXECUTE_OBJ->handlers.resource_execute = valid_args_with_values_execute;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("128", "514", "1"), PAYLOAD("0,1='value',2"));

    _anjay_mock_dm_expect_list_instances(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

static int
valid_values_partial_read_execute(anjay_t *anjay,
                                  const anjay_dm_object_def_t *const *obj_ptr,
                                  anjay_iid_t iid,
                                  anjay_rid_t rid,
                                  anjay_execute_ctx_t *ctx) {
    (void) iid;
    (void) rid;
    (void) anjay;
    (void) obj_ptr;
    int ret;
    int arg;
    bool has_value;

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 1);
    AVS_UNIT_ASSERT_EQUAL(has_value, true);

    char buf[32];
    /* Read in 2 parts. */
    size_t read_bytes;
    AVS_UNIT_ASSERT_EQUAL(anjay_execute_get_arg_value(ctx, &read_bytes, buf, 5),
                          ANJAY_BUFFER_TOO_SHORT);
    AVS_UNIT_ASSERT_EQUAL(read_bytes, strlen("very"));
    AVS_UNIT_ASSERT_EQUAL_STRING(buf, "very");
    AVS_UNIT_ASSERT_SUCCESS(
            anjay_execute_get_arg_value(ctx, &read_bytes, buf, 32));
    AVS_UNIT_ASSERT_EQUAL(read_bytes, strlen("longvalue"));
    AVS_UNIT_ASSERT_EQUAL_STRING(buf, "longvalue");

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 1);
    AVS_UNIT_ASSERT_EQUAL(arg, -1);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);
    return 0;
}

AVS_UNIT_TEST(dm_execute, valid_values_partial_read) {
    DM_TEST_INIT;
    EXECUTE_OBJ->handlers.resource_execute = valid_values_partial_read_execute;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("128", "514", "1"), PAYLOAD("1='verylongvalue'"));

    _anjay_mock_dm_expect_list_instances(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

static int
valid_values_skipping_execute(anjay_t *anjay,
                              const anjay_dm_object_def_t *const *obj_ptr,
                              anjay_iid_t iid,
                              anjay_rid_t rid,
                              anjay_execute_ctx_t *ctx) {
    (void) iid;
    (void) rid;
    (void) anjay;
    (void) obj_ptr;
    int ret;
    int arg;
    bool has_value;

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 1);
    AVS_UNIT_ASSERT_EQUAL(has_value, true);

    char buf[2];
    size_t bytes_read;
    AVS_UNIT_ASSERT_EQUAL(anjay_execute_get_arg_value(ctx, &bytes_read, buf, 2),
                          ANJAY_BUFFER_TOO_SHORT);
    AVS_UNIT_ASSERT_EQUAL(bytes_read, 1);
    /* Don't care about the rest, ignore. */
    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 2);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 0);
    AVS_UNIT_ASSERT_EQUAL(arg, 3);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    AVS_UNIT_ASSERT_SUCCESS(
            anjay_execute_get_arg_value(ctx, &bytes_read, buf, 2));
    AVS_UNIT_ASSERT_EQUAL(bytes_read, 0);

    ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    AVS_UNIT_ASSERT_EQUAL(ret, 1);
    AVS_UNIT_ASSERT_EQUAL(arg, -1);
    AVS_UNIT_ASSERT_EQUAL(has_value, false);

    return 0;
}

AVS_UNIT_TEST(dm_execute, valid_values_skipping) {
    DM_TEST_INIT;
    EXECUTE_OBJ->handlers.resource_execute = valid_values_skipping_execute;
    DM_TEST_REQUEST(
            mocksocks[0], CON, POST, ID(0xFA3E), PATH("128", "514", "1"),
            PAYLOAD("1='ludicrously-long-value-because-we-want-this-to-not-fit-"
                    "inside-a-single-64-byte-buffer-that-we-use-for-skipping-"
                    "unread-values-and-also-the-spec-says-that-spaces-are-"
                    "illegal-inside-Execute-arguments-so-thanks-OMA',2,3"));

    _anjay_mock_dm_expect_list_instances(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

static int invalid_input_execute(anjay_t *anjay,
                                 const anjay_dm_object_def_t *const *obj_ptr,
                                 anjay_iid_t iid,
                                 anjay_rid_t rid,
                                 anjay_execute_ctx_t *ctx) {
    (void) iid;
    (void) rid;
    (void) anjay;
    (void) obj_ptr;
    int ret;
    int arg;
    bool has_value;
    char arg_value_buffer[2];

    do {
        ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
        // anjay_execute_get_arg_value() should never trigger an error other
        // than ANJAY_ERR_BAD_REQUEST for valid function arguments
        int get_arg_value_ret =
                anjay_execute_get_arg_value(ctx, NULL, arg_value_buffer,
                                            AVS_ARRAY_SIZE(arg_value_buffer));
        AVS_UNIT_ASSERT_TRUE(get_arg_value_ret == 0
                             || get_arg_value_ret == ANJAY_BUFFER_TOO_SHORT
                             || get_arg_value_ret == ANJAY_ERR_BAD_REQUEST);
    } while (!ret);

    return ret == ANJAY_EXECUTE_GET_ARG_END ? 0 : ret;
}

AVS_UNIT_TEST(dm_execute, invalid_input) {
    // clang-format off
    static const char* invalid_inputs[] = {
        "a",
        "0=",
        "0=1,2,3",
        "0='val,1",
        "0='val',1='val',3'',4",
        "=",
        "11",
        "0='val',11",
        "0='val",
        "0=1=",
        ",0",
        ",,0",
        "0,",
        "0,,",
        "0=,",
        ",0=",
        "0='\"'"
    };
    // clang-format on

    EXECUTE_OBJ->handlers.resource_execute = invalid_input_execute;
    for (size_t i = 0; i < AVS_ARRAY_SIZE(invalid_inputs); i++) {
        DM_TEST_INIT;
        DM_TEST_REQUEST(
                mocksocks[0], CON, POST, ID(0xFA3E), PATH("128", "514", "1"),
                PAYLOAD_EXTERNAL(invalid_inputs[i], strlen(invalid_inputs[i])));
        _anjay_mock_dm_expect_list_instances(
                anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 0,
                (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
        _anjay_mock_dm_expect_list_resources(
                anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 514,
                0,
                (const anjay_mock_dm_res_entry_t[]) {
                        { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                        { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        ANJAY_MOCK_DM_RES_END });
        DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_REQUEST, ID(0xFA3E),
                                NO_PAYLOAD);
        expect_has_buffered_data_check(mocksocks[0], false);
        AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
        DM_TEST_FINISH;
    }
}

static int valid_input_execute(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *obj_ptr,
                               anjay_iid_t iid,
                               anjay_rid_t rid,
                               anjay_execute_ctx_t *ctx) {
    (void) iid;
    (void) rid;
    (void) anjay;
    (void) obj_ptr;
    int ret;
    int arg;
    bool has_value;

    do {
        ret = anjay_execute_get_next_arg(ctx, &arg, &has_value);
    } while (!ret);

    return ret < 0 ? -1 : 0;
}

AVS_UNIT_TEST(dm_execute, valid_input) {
    static const char *valid_inputs[] = { "", "0='ala'", "2='10.3'",
                                          "7,0='https://www.oma.org'",
                                          "0,1,2,3,4" };

    EXECUTE_OBJ->handlers.resource_execute = valid_input_execute;
    for (size_t i = 0; i < AVS_ARRAY_SIZE(valid_inputs); i++) {
        DM_TEST_INIT;
        DM_TEST_REQUEST(
                mocksocks[0], CON, POST, ID(0xFA3E), PATH("128", "514", "1"),
                PAYLOAD_EXTERNAL(valid_inputs[i], strlen(valid_inputs[i])));
        _anjay_mock_dm_expect_list_instances(
                anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 0,
                (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
        _anjay_mock_dm_expect_list_resources(
                anjay, (const anjay_dm_object_def_t *const *) &EXECUTE_OBJ, 514,
                0,
                (const anjay_mock_dm_res_entry_t[]) {
                        { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                        { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                        ANJAY_MOCK_DM_RES_END });
        DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E),
                                NO_PAYLOAD);
        expect_has_buffered_data_check(mocksocks[0], false);
        AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
        DM_TEST_FINISH;
    }
}

AVS_UNIT_TEST(dm_write_attributes, resource) {
    DM_TEST_INIT_WITH_SSIDS(77);
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "514", "4"),
                    QUERY("pmin=42", "st=0.7", "epmax=2"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 514, 4, 77, 0,
                                              &ANJAY_DM_R_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_resource_write_attrs(
            anjay, &OBJ, 514, 4, 77,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 42,
                    .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_eval_period = 2
#ifdef ANJAY_WITH_CON_ATTR
                    ,
                    .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = 0.7
            },
            0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write_attributes, instance) {
    DM_TEST_INIT_WITH_SSIDS(42);
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "77"),
                    QUERY("pmin=69", "epmin=70"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 69, 77, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 77, 42, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_instance_write_default_attrs(
            anjay, &OBJ, 77, 42,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 69,
                .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                .min_eval_period = 70,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            },
            0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write_attributes, object) {
    DM_TEST_INIT_WITH_SSIDS(666);
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42"),
                    QUERY("pmax=514", "epmin=10", "epmax=20"));
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 666, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_object_write_default_attrs(
            anjay, &OBJ, 666,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_period = 514,
                .min_eval_period = 10,
                .max_eval_period = 20
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            },
            0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write_attributes, no_resource) {
    DM_TEST_INIT_WITH_SSIDS(1);
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "2", "3"),
                    QUERY("pmin=42"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 2, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 2, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write_attributes, no_instance) {
    DM_TEST_INIT_WITH_SSIDS(4);
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "5", "6"),
                    QUERY("pmin=42"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 2, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write_attributes, negative_pmin) {
    DM_TEST_INIT_WITH_SSIDS(42);
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "77"),
                    QUERY("pmin=-1"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_OPTION, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_write_attributes, negative_pmax) {
    DM_TEST_INIT_WITH_SSIDS(42);
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "77"),
                    QUERY("pmax=-1"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_OPTION, ID(0xFA3E),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

#ifdef ANJAY_WITH_LWM2M11
AVS_UNIT_TEST(dm_discover, resource_instance_attrs) {
    DM_TEST_INIT_WITH_SSIDS(34, 45);
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    anjay_unlocked->servers->registration_info.lwm2m_version =
            ANJAY_LWM2M_VERSION_1_1;
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 4, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    const anjay_riid_t resource_instances[] = { 123, 456, 789,
                                                ANJAY_ID_INVALID };

    // First call of dm_list_resource_instances - compute resource dim
    _anjay_mock_dm_expect_list_resource_instances(anjay, &OBJ, 69, 4, 0,
                                                  resource_instances);

    _anjay_mock_dm_expect_resource_read_attrs(
            anjay, &OBJ, 69, 4, 34, 0,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 10,
                    .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_eval_period = 5
#    ifdef ANJAY_WITH_CON_ATTR
                    ,
                    .con = ANJAY_DM_CON_ATTR_NONE
#    endif // ANJAY_WITH_CON_ATTR
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 34, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 34, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);

    // Second call of dm_list_resource_instances - actually print riids
    _anjay_mock_dm_expect_list_resource_instances(anjay, &OBJ, 69, 4, 0,
                                                  resource_instances);
    for (const anjay_riid_t *riid = resource_instances;
         *riid != ANJAY_ID_INVALID;
         riid++) {
        _anjay_mock_dm_expect_resource_instance_read_attrs(
                anjay, &OBJ, 69, 4, *riid, 34, 0,
                &(const anjay_dm_r_attributes_t) {
                    .common = {
                        .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                        .max_period = *riid,
                        .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                        .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#    ifdef ANJAY_WITH_CON_ATTR
                        ,
                        .con = ANJAY_DM_CON_ATTR_NONE
#    endif // ANJAY_WITH_CON_ATTR
                    },
                    .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                    .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                    .step = ANJAY_ATTRIB_DOUBLE_NONE
                });
    }

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xfa3e),
                            CONTENT_FORMAT(LINK_FORMAT),
                            PAYLOAD("</42/69/4>;dim=3;pmin=10;epmax=5,"
                                    "</42/69/4/123>;pmax=123,"
                                    "</42/69/4/456>;pmax=456,"
                                    "</42/69/4/789>;pmax=789"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}
#endif // ANJAY_WITH_LWM2M11

AVS_UNIT_TEST(dm_discover, resource) {
    DM_TEST_INIT_WITH_SSIDS(7);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read_attrs(
            anjay, &OBJ, 69, 4, 7, 0,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_period = 514,
                    .min_eval_period = 25,
                    .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                    ,
                    .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = 6.46,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });

    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 7, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });

    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 7, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 10,
                .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });

    DM_TEST_EXPECT_RESPONSE(
            mocksocks[0], ACK, CONTENT, ID(0xfa3e), CONTENT_FORMAT(LINK_FORMAT),
            PAYLOAD("</42/69/4>;pmin=10;pmax=514;epmin=25;lt=6.46"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

#ifdef ANJAY_WITH_LWM2M11
AVS_UNIT_TEST(dm_discover, resource_multiple_servers) {
    DM_TEST_INIT_WITH_SSIDS(34, 45);
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    anjay_unlocked->servers->registration_info.lwm2m_version =
            ANJAY_LWM2M_VERSION_1_1;
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RWM, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    const anjay_riid_t resource_instances[] = { 0, 1, 2, ANJAY_ID_INVALID };

    // First call of dm_list_resource_instances - compute resource dim
    _anjay_mock_dm_expect_list_resource_instances(anjay, &OBJ, 69, 4, 0,
                                                  resource_instances);

    _anjay_mock_dm_expect_resource_read_attrs(
            anjay, &OBJ, 69, 4, 34, 0,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 10,
                    .max_period = 514,
                    .min_eval_period = 3,
                    .max_eval_period = 600
#    ifdef ANJAY_WITH_CON_ATTR
                    ,
                    .con = ANJAY_DM_CON_ATTR_NONE
#    endif // ANJAY_WITH_CON_ATTR
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = 6.46,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
#    ifdef ANJAY_WITH_CON_ATTR
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 34, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 34, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
#    endif // ANJAY_WITH_CON_ATTR

    // Second call of dm_list_resource_instances - actually print riids
    _anjay_mock_dm_expect_list_resource_instances(anjay, &OBJ, 69, 4, 0,
                                                  resource_instances);
    for (const anjay_riid_t *riid = resource_instances;
         *riid != ANJAY_ID_INVALID;
         riid++) {
        _anjay_mock_dm_expect_resource_instance_read_attrs(
                anjay, &OBJ, 69, 4, *riid, 34, 0, &ANJAY_DM_R_ATTRIBUTES_EMPTY);
    }

    DM_TEST_EXPECT_RESPONSE(
            mocksocks[0], ACK, CONTENT, ID(0xfa3e), CONTENT_FORMAT(LINK_FORMAT),
            PAYLOAD("</42/69/"
                    "4>;dim=3;pmin=10;pmax=514;epmin=3;epmax=600;"
                    "lt=6.46,</42/69/4/0>,</42/69/4/1>,</42/69/4/2>"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}
#endif // ANJAY_WITH_LWM2M11

AVS_UNIT_TEST(dm_discover, instance) {
    DM_TEST_INIT_WITH_SSIDS(69);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "514"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });

    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 514, 69, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_period = 777,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 69, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 666,
                .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    for (anjay_iid_t rid = 0; rid < 2; ++rid) {
        anjay_dm_r_attributes_t attrs = ANJAY_DM_R_ATTRIBUTES_EMPTY;
        attrs.greater_than = (double) rid;
        _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 514, rid, 69, 0,
                                                  &attrs);
    }

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xfa3e),
                            CONTENT_FORMAT(LINK_FORMAT),
                            PAYLOAD("</42/514>;pmin=666;pmax=777,"
                                    "</42/514/0>;gt=0,</42/514/1>;gt=1"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_discover, instance_multiple_servers) {
    DM_TEST_INIT_WITH_SSIDS(69, 96);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "514"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });

    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 514, 69, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_period = 777,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 69, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 666,
                .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    for (anjay_iid_t rid = 0; rid < 2; ++rid) {
        anjay_dm_r_attributes_t attrs = ANJAY_DM_R_ATTRIBUTES_EMPTY;
        attrs.greater_than = (double) rid;
        _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 514, rid, 69, 0,
                                                  &attrs);
    }

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xfa3e),
                            CONTENT_FORMAT(LINK_FORMAT),
                            PAYLOAD("</42/514>;pmin=666;pmax=777,"
                                    "</42/514/0>;gt=0,</42/514/1>;gt=1"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

static void test_discover_object(anjay_t *anjay, avs_net_socket_t *mocksock) {
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 2, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_period = 514,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });

    const anjay_mock_dm_res_entry_t *resources[] = {
        (const anjay_mock_dm_res_entry_t[]) {
                { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                ANJAY_MOCK_DM_RES_END },
        (const anjay_mock_dm_res_entry_t[]) {
                { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                ANJAY_MOCK_DM_RES_END }
    };
    const size_t ITERATIONS = AVS_ARRAY_SIZE(resources);
    anjay_iid_t iids[ITERATIONS + 1];
    for (anjay_iid_t iid = 0; iid < ITERATIONS; ++iid) {
        iids[iid] = iid;
    }
    iids[ITERATIONS] = ANJAY_ID_INVALID;
    _anjay_mock_dm_expect_list_instances(anjay, &OBJ, 0, iids);
    for (anjay_iid_t iid = 0; iid < ITERATIONS; ++iid) {
        _anjay_mock_dm_expect_list_resources(anjay, &OBJ, iid, 0,
                                             resources[iid]);
    }

    DM_TEST_EXPECT_RESPONSE(mocksock, ACK, CONTENT, ID(0xfa3e),
                            CONTENT_FORMAT(LINK_FORMAT),
                            PAYLOAD("</42>;pmax=514,</42/0>,</42/0/0>,"
                                    "</42/0/3>,</42/0/4>,</42/0/6>,</42/1>,"
                                    "</42/1/4>,</42/1/5>,</42/1/6>"));
    expect_has_buffered_data_check(mocksock, false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksock));
}

AVS_UNIT_TEST(dm_discover, object) {
    DM_TEST_INIT_WITH_SSIDS(2);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42"),
                    ACCEPT(0x28), NO_PAYLOAD);
    test_discover_object(anjay, mocksocks[0]);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_discover, object_multiple_servers) {
    DM_TEST_INIT_WITH_SSIDS(2, 3);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 2, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_period = 514,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                ,
                .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
            });

    const anjay_mock_dm_res_entry_t *resources[] = {
        (const anjay_mock_dm_res_entry_t[]) {
                { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                ANJAY_MOCK_DM_RES_END },
        (const anjay_mock_dm_res_entry_t[]) {
                { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                ANJAY_MOCK_DM_RES_END }
    };
    const size_t ITERATIONS = AVS_ARRAY_SIZE(resources);
    anjay_iid_t iids[ITERATIONS + 1];
    for (anjay_iid_t iid = 0; iid < ITERATIONS; ++iid) {
        iids[iid] = iid;
    }
    iids[ITERATIONS] = ANJAY_ID_INVALID;
    _anjay_mock_dm_expect_list_instances(anjay, &OBJ, 0, iids);
    for (anjay_iid_t iid = 0; iid < ITERATIONS; ++iid) {
        _anjay_mock_dm_expect_list_resources(anjay, &OBJ, iid, 0,
                                             resources[iid]);
    }

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xfa3e),
                            CONTENT_FORMAT(LINK_FORMAT),
                            PAYLOAD("</42>;pmax=514,</42/0>,</42/0/0>,"
                                    "</42/0/3>,</42/0/4>,</42/0/6>,</42/1>,"
                                    "</42/1/4>,</42/1/5>,</42/1/6>"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_discover, error) {
    DM_TEST_INIT_WITH_SSIDS(7);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 69, 4, 7,
                                              ANJAY_ERR_INTERNAL,
                                              &ANJAY_DM_R_ATTRIBUTES_EMPTY);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xfa3e), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_discover, multiple_servers_empty) {
    DM_TEST_INIT_WITH_SSIDS(34, 45);
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    ACCEPT(0x28), NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 69, 4, 34, 0,
                                              &ANJAY_DM_R_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 34, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 34, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);

    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xfa3e),
                            CONTENT_FORMAT(LINK_FORMAT), PAYLOAD("</42/69/4>"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_create, only_iid) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x20"
                            "\x02\x02"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_create(anjay, &OBJ, 514, 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CREATED, ID(0xFA3E),
                            LOCATION_PATH("42", "514"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_create, failure) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x20"
                            "\x02\x02"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_create(anjay, &OBJ, 514, -1);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xfa3e), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_create, already_exists) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x00"
                            "\x45"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_REQUEST, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_create, no_iid) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV), NO_PAYLOAD);

    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 0, 1, 2, 3, 4, 5, 7, 8, 9,
                                    ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_create(anjay, &OBJ, 6, 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CREATED, ID(0xFA3E),
                            LOCATION_PATH("42", "6"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_create, with_data) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 0, 1, 3, 4, 5, 6, 7, 8, 9,
                                    ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_create(anjay, &OBJ, 2, 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 2, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 2, 0, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 13), 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 2, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 2, 6, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CREATED, ID(0xFA3E),
                            LOCATION_PATH("42", "2"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_create, with_iid_and_data) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x08\x45\x0a"
                            "\xc1\x00\x0d"
                            "\xc5\x06"
                            "Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 4, 14, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_create(anjay, &OBJ, 69, 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 69, 0, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 13), 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 69, 6, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CREATED, ID(0xFA3E),
                            LOCATION_PATH("42", "69"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_create, multiple_iids) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42"),
                    CONTENT_FORMAT(OMA_LWM2M_TLV),
                    PAYLOAD("\x08\x45\x03"
                            "\xc1\x00\x2a"
                            "\x08\x2a\x03"
                            "\xc1\x03\x45"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 4, 14, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_create(anjay, &OBJ, 69, 0);
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 69, 0, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_INT(0, 42), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, BAD_REQUEST, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_delete, success) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, DELETE, ID(0xFA3E), PATH("42", "34"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 34, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_remove(anjay, &OBJ, 34, 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, DELETED, ID(0xfa3e), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_delete, no_iid) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, DELETE, ID(0xFA3E), PATH("42"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_delete, superfluous_rid) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, DELETE, ID(0xFA3E),
                    PATH("42", "514", "2"));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_delete, not_exists) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, DELETE, ID(0xFA3E), PATH("42", "69"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 34, ANJAY_ID_INVALID });
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, NOT_FOUND, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_delete, failure) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, DELETE, ID(0xFA3E), PATH("42", "84"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0, (const anjay_iid_t[]) { 84, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_instance_remove(anjay, &OBJ, 84, ANJAY_ERR_INTERNAL);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, INTERNAL_SERVER_ERROR,
                            ID(0xfa3e), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_FAILED(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

#warning \
        "TODO: requires proper setup of server connection along with CoAP2 streaming context"
#if 0
static int setup_response_succeed(avs_stream_t *stream,
                                  const anjay_msg_details_t *details) {
    (void) stream;
    (void) details;
    return 0;
}

static int mock_get_path(anjay_input_ctx_t *in_ctx,
                         anjay_uri_path_t *out_path,
                         bool *out_is_array) {
    (void) in_ctx;
    *out_path = MAKE_RESOURCE_PATH(ANJAY_ID_INVALID, ANJAY_ID_INVALID, 0);
    *out_is_array = false;
    return 0;
}

static int fail() {
    AVS_UNIT_ASSERT_TRUE(false);
    return -1;
}

typedef struct coap_stream_mock {
    const avs_stream_v_table_t *vtable;
} coap_stream_mock_t;

AVS_UNIT_TEST(dm_operations, unimplemented) {
    anjay_t anjay;
    memset(&anjay, 0, sizeof(anjay));

    coap_stream_mock_t mock = {
        .vtable = &(const avs_stream_v_table_t) {
            .write_some = (avs_stream_write_some_t) fail,
            .finish_message = (avs_stream_finish_message_t) fail,
            .read = (avs_stream_read_t) fail,
            .peek = (avs_stream_peek_t) fail,
            .reset = (avs_stream_reset_t) fail,
            .close = (avs_stream_close_t) fail,
            .get_errno = (avs_stream_errno_t) fail
        }
    };

    const anjay_dm_object_def_t OBJ_DEF = {
        .oid = 1337
    };
    const anjay_dm_object_def_t *const def_ptr = &OBJ_DEF;

    anjay_input_ctx_vtable_t in_ctx_vtable = {
        .some_bytes = (anjay_input_ctx_bytes_t) fail,
        .string = (anjay_input_ctx_string_t) fail,
        .integer = (anjay_input_ctx_integer_t) fail,
        .floating = (anjay_input_ctx_floating_t) fail,
        .boolean = (anjay_input_ctx_boolean_t) fail,
        .objlnk = (anjay_input_ctx_objlnk_t) fail,
        .get_path = (anjay_input_ctx_get_path_t) fail,
        .close = (anjay_input_ctx_close_t) fail
    };
    struct {
        const anjay_input_ctx_vtable_t *vtable;
    } in_ctx = {
        .vtable = &in_ctx_vtable
    };

#    define ASSERT_ACTION_FAILS(...)                                         \
        {                                                                    \
            avs_coap_msg_identity_t request_identity = { 0 };                \
            anjay_request_t request = {                                      \
                .requested_format = AVS_COAP_FORMAT_NONE,                    \
                .action = __VA_ARGS__                                        \
            };                                                               \
            AVS_UNIT_ASSERT_FAILED(                                          \
                    invoke_action(&anjay, &def_ptr, &request_identity,       \
                                  &request, (anjay_input_ctx_t *) &in_ctx)); \
        }

    anjay.comm_stream = (avs_stream_t *) &mock;
    AVS_UNIT_MOCK(_anjay_coap_stream_setup_response) = setup_response_succeed;
    anjay.current_connection.server = &(anjay_server_info_t) {
        .ssid = 0
    };
    anjay_uri_path_t uri_object = MAKE_OBJECT_PATH(1337);
    anjay_uri_path_t uri_instance = MAKE_INSTANCE_PATH(1337, 0);
    anjay_uri_path_t uri_resource = MAKE_RESOURCE_PATH(1337, 0, 0);
    ASSERT_ACTION_FAILS(ANJAY_ACTION_READ,
                        .uri = uri_resource)
    ASSERT_ACTION_FAILS(ANJAY_ACTION_DISCOVER,
                        .uri = uri_resource)
    ASSERT_ACTION_FAILS(ANJAY_ACTION_WRITE,
                        .uri = uri_resource)
    ASSERT_ACTION_FAILS(ANJAY_ACTION_WRITE_UPDATE,
                        .uri = uri_resource)
    ASSERT_ACTION_FAILS(ANJAY_ACTION_WRITE_ATTRIBUTES,
                        .uri = uri_resource,
                        .attributes = {
                            .has_min_period = true
                        })
    ASSERT_ACTION_FAILS(ANJAY_ACTION_EXECUTE,
                        .uri = uri_resource)
    ASSERT_ACTION_FAILS(ANJAY_ACTION_DELETE,
                        .uri = uri_instance)
    in_ctx_vtable.get_path = mock_get_path;
    ASSERT_ACTION_FAILS(ANJAY_ACTION_CREATE,
                        .uri = uri_object)

    // Cancel Observe does not call any handlers, so it does not fail
}
#endif

static const anjay_dm_attrs_query_details_t
        DM_EFFECTIVE_ATTRS_STANDARD_QUERY = {
            .obj = &(const anjay_dm_installed_object_t) {
#ifdef ANJAY_WITH_THREAD_SAFETY
                .type = ANJAY_DM_OBJECT_USER_PROVIDED,
                .impl.user_provided =
#endif // ANJAY_WITH_THREAD_SAFETY
                        &OBJ
            },
            .iid = 69,
            .rid = 4,
            .riid = ANJAY_ID_INVALID,
            .ssid = 1,
            .with_server_level_attrs = true
        };

AVS_UNIT_TEST(dm_effective_attrs, resource_full) {
    DM_TEST_INIT;
    (void) mocksocks;
    static const anjay_dm_r_attributes_t RES_ATTRS = {
        .common = {
            .min_period = 14,
            .max_period = 42,
            .min_eval_period = 99,
            .max_eval_period = 150
        },
        .greater_than = 77.2,
        .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
        .step = ANJAY_ATTRIB_DOUBLE_NONE
    };
    _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 69, 4, 1, 0,
                                              &RES_ATTRS);

    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    anjay_dm_r_attributes_t attrs;
    AVS_UNIT_ASSERT_SUCCESS(_anjay_dm_effective_attrs(
            anjay_unlocked, &DM_EFFECTIVE_ATTRS_STANDARD_QUERY, &attrs));
    _anjay_mock_dm_assert_attributes_equal(&attrs, &RES_ATTRS);
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, fallback_to_instance) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_resource_read_attrs(
            anjay, &OBJ, 69, 4, 1, 0,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 14,
                    .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .min_eval_period = 15,
                    .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 1, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 514,
                .max_period = 42,
                .min_eval_period = 99,
                .max_eval_period = 190
            });
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    anjay_dm_r_attributes_t attrs;
    AVS_UNIT_ASSERT_SUCCESS(_anjay_dm_effective_attrs(
            anjay_unlocked, &DM_EFFECTIVE_ATTRS_STANDARD_QUERY, &attrs));
    _anjay_mock_dm_assert_attributes_equal(
            &attrs,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 14,
                    .max_period = 42,
                    .min_eval_period = 15,
                    .max_eval_period = 190
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, fallback_to_object) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_resource_read_attrs(
            anjay, &OBJ, 69, 4, 1, 0,
            &(const anjay_dm_r_attributes_t) {
                .common = ANJAY_DM_OI_ATTRIBUTES_EMPTY,
                .greater_than = 43.7,
                .less_than = 17.3,
                .step = 6.9
            });
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 1, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_period = 777,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
            });
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 514,
                .max_period = 69,
                .min_eval_period = 100,
                .max_eval_period = 800
            });
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    anjay_dm_r_attributes_t attrs;
    AVS_UNIT_ASSERT_SUCCESS(_anjay_dm_effective_attrs(
            anjay_unlocked, &DM_EFFECTIVE_ATTRS_STANDARD_QUERY, &attrs));
    _anjay_mock_dm_assert_attributes_equal(&attrs,
                                           &(const anjay_dm_r_attributes_t) {
                                               .common = {
                                                   .min_period = 514,
                                                   .max_period = 777,
                                                   .min_eval_period = 100,
                                                   .max_eval_period = 800
                                               },
                                               .greater_than = 43.7,
                                               .less_than = 17.3,
                                               .step = 6.9
                                           });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, fallback_to_server) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 69, 4, 1, 0,
                                              &ANJAY_DM_R_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 1, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 4,
                .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
            });
    _anjay_mock_dm_expect_list_instances(
            anjay, &FAKE_SERVER, 0,
            (const anjay_iid_t[]) { 1, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_SSID,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 1));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_DEFAULT_PMAX,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 42));
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    anjay_dm_r_attributes_t attrs;
    AVS_UNIT_ASSERT_SUCCESS(_anjay_dm_effective_attrs(
            anjay_unlocked, &DM_EFFECTIVE_ATTRS_STANDARD_QUERY, &attrs));
    _anjay_mock_dm_assert_attributes_equal(
            &attrs,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 4,
                    .max_period = 42,
                    .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, resource_fail) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_resource_read_attrs(anjay, &OBJ, 69, 4, 1, -1, NULL);
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    anjay_dm_r_attributes_t attrs = ANJAY_DM_R_ATTRIBUTES_EMPTY;
    AVS_UNIT_ASSERT_FAILED(_anjay_dm_effective_attrs(
            anjay_unlocked, &DM_EFFECTIVE_ATTRS_STANDARD_QUERY, &attrs));
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, for_instance) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_instance_read_default_attrs(
            anjay, &OBJ, 69, 1, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 9,
                .max_period = 77,
                .min_eval_period = 10,
                .max_eval_period = 88
            });
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_SUCCESS(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    _anjay_mock_dm_assert_attributes_equal(
            &attrs,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 9,
                    .max_period = 77,
                    .min_eval_period = 10,
                    .max_eval_period = 88
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, instance_fail) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_instance_read_default_attrs(anjay, &OBJ, 69, 1, -1,
                                                      NULL);
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_FAILED(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, for_object) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0,
            &(const anjay_dm_oi_attributes_t) {
                .min_period = 6,
                .max_period = 54,
                .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
            });
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    details.iid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_SUCCESS(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    _anjay_mock_dm_assert_attributes_equal(
            &attrs,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 6,
                    .max_period = 54,
                    .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, object_fail) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_object_read_default_attrs(anjay, &OBJ, 1, -1, NULL);
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    details.iid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_FAILED(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, server_default) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_list_instances(
            anjay, &FAKE_SERVER, 0,
            (const anjay_iid_t[]) { 1, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_SSID,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 1));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_DEFAULT_PMIN,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 0));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_DEFAULT_PMAX,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 404));
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    details.iid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_SUCCESS(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    _anjay_mock_dm_assert_attributes_equal(
            &attrs,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = 0,
                    .max_period = 404,
                    .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                    ,
                    .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, no_server) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_list_instances(
            anjay, &FAKE_SERVER, 0, (const anjay_iid_t[]) { ANJAY_ID_INVALID });
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    details.iid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_SUCCESS(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    _anjay_mock_dm_assert_attributes_equal(
            &attrs,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = ANJAY_DM_DEFAULT_PMIN_VALUE,
                    .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                    ,
                    .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, no_resources) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_list_instances(
            anjay, &FAKE_SERVER, 0,
            (const anjay_iid_t[]) { 1, ANJAY_ID_INVALID });

    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });

    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_SSID,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 1));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    details.iid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_SUCCESS(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    _anjay_mock_dm_assert_attributes_equal(
            &attrs,
            &(const anjay_dm_r_attributes_t) {
                .common = {
                    .min_period = ANJAY_DM_DEFAULT_PMIN_VALUE,
                    .max_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .min_eval_period = ANJAY_ATTRIB_INTEGER_NONE,
                    .max_eval_period = ANJAY_ATTRIB_INTEGER_NONE
#ifdef ANJAY_WITH_CON_ATTR
                    ,
                    .con = ANJAY_DM_CON_ATTR_NONE
#endif // ANJAY_WITH_CON_ATTR
                },
                .greater_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .less_than = ANJAY_ATTRIB_DOUBLE_NONE,
                .step = ANJAY_ATTRIB_DOUBLE_NONE
            });
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, read_error) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_list_instances(
            anjay, &FAKE_SERVER, 0,
            (const anjay_iid_t[]) { 1, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_SSID,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 1));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_DEFAULT_PMIN,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 7));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_DEFAULT_PMAX,
                                        ANJAY_ID_INVALID, -1,
                                        ANJAY_MOCK_DM_NONE);
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    details.iid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_FAILED(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_effective_attrs, read_invalid) {
    DM_TEST_INIT;
    (void) mocksocks;
    _anjay_mock_dm_expect_object_read_default_attrs(
            anjay, &OBJ, 1, 0, &ANJAY_DM_OI_ATTRIBUTES_EMPTY);
    _anjay_mock_dm_expect_list_instances(
            anjay, &FAKE_SERVER, 0,
            (const anjay_iid_t[]) { 1, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_SSID,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 1));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_DEFAULT_PMIN,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 7));
    _anjay_mock_dm_expect_list_resources(
            anjay, &FAKE_SERVER, 1, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { ANJAY_DM_RID_SERVER_SSID, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_LIFETIME, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMIN, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_DEFAULT_PMAX, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT },
                    { ANJAY_DM_RID_SERVER_NOTIFICATION_STORING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    { ANJAY_DM_RID_SERVER_BINDING, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &FAKE_SERVER, 1,
                                        ANJAY_DM_RID_SERVER_DEFAULT_PMAX,
                                        ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, -1));
    anjay_dm_r_attributes_t attrs;
    anjay_dm_attrs_query_details_t details = DM_EFFECTIVE_ATTRS_STANDARD_QUERY;
    details.rid = ANJAY_ID_INVALID;
    details.iid = ANJAY_ID_INVALID;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_FAILED(
            _anjay_dm_effective_attrs(anjay_unlocked, &details, &attrs));
    ANJAY_MUTEX_UNLOCK(anjay);
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_resource_operations, nonreadable_resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    // 4.05 Method Not Allowed
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_resource_operations, nonexecutable_resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E), PATH("42", "69", "4"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_W, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_E, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    // 4.05 Method Not Allowed
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_resource_operations, nonwritable_resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "69", "4"),
                    CONTENT_FORMAT(PLAINTEXT), PAYLOAD("content"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    // 4.05 Method Not Allowed
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, METHOD_NOT_ALLOWED, ID(0xfa3e),
                            NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_resource_operations, readable_resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, GET, ID(0xFA3E), PATH("42", "69", "4"),
                    NO_PAYLOAD);
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 4, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_INT(0, 514));
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CONTENT, ID(0xFA3E),
                            CONTENT_FORMAT(PLAINTEXT), PAYLOAD("514"));
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_resource_operations, executable_resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, POST, ID(0xFA3E),
                    PATH("42", "514", "4"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_execute(anjay, &OBJ, 514, 4, NULL, 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_resource_operations, writable_resource) {
    DM_TEST_INIT;
    DM_TEST_REQUEST(mocksocks[0], CON, PUT, ID(0xFA3E), PATH("42", "514", "4"),
                    CONTENT_FORMAT(PLAINTEXT), PAYLOAD("Hello"));
    _anjay_mock_dm_expect_list_instances(
            anjay, &OBJ, 0,
            (const anjay_iid_t[]) { 14, 42, 69, 514, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_W, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_R, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_write(anjay, &OBJ, 514, 4, ANJAY_ID_INVALID,
                                         ANJAY_MOCK_DM_STRING(0, "Hello"), 0);
    DM_TEST_EXPECT_RESPONSE(mocksocks[0], ACK, CHANGED, ID(0xFA3E), NO_PAYLOAD);
    expect_has_buffered_data_check(mocksocks[0], false);
    AVS_UNIT_ASSERT_SUCCESS(anjay_serve(anjay, mocksocks[0]));
    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_res_read, no_space) {
    DM_TEST_INIT;

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 42, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 42, 3, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_STRING(0, ""));
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_SUCCESS(_anjay_dm_read_resource_into_buffer(
            anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 42, 3), NULL, 0,
            NULL));
    ANJAY_MUTEX_UNLOCK(anjay);

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 514, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 514, 4, ANJAY_ID_INVALID,
                                        -1, ANJAY_MOCK_DM_STRING(-1, "Hello"));
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_FAILED(_anjay_dm_read_resource_into_buffer(
            anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 514, 4), NULL, 0,
            NULL));
    ANJAY_MUTEX_UNLOCK(anjay);

    char fake_string = 42;
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 69, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 69, 5, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_STRING(0, ""));
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_SUCCESS(_anjay_dm_read_resource_string(
            anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 69, 5), &fake_string,
            1));
    ANJAY_MUTEX_UNLOCK(anjay);
    AVS_UNIT_ASSERT_EQUAL(fake_string, 0);

    fake_string = 69;
    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 32, 0,
            (const anjay_mock_dm_res_entry_t[]) {
                    { 0, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 1, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 2, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 3, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 4, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 5, ANJAY_DM_RES_RW, ANJAY_DM_RES_ABSENT },
                    { 6, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT },
                    ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 32, 6, ANJAY_ID_INVALID,
                                        -1,
                                        ANJAY_MOCK_DM_STRING(-1, "Goodbye"));
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    AVS_UNIT_ASSERT_FAILED(_anjay_dm_read_resource_string(
            anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 32, 6), &fake_string,
            1));
    ANJAY_MUTEX_UNLOCK(anjay);
    AVS_UNIT_ASSERT_EQUAL(fake_string, 69);

    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_res_read, objlnk) {
    DM_TEST_INIT;

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 42, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 1, ANJAY_DM_RES_R,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });

    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 42, 1, ANJAY_ID_INVALID, 0,
                                        ANJAY_MOCK_DM_OBJLNK(0, 123, 456));

    anjay_oid_t oid = 0;
    anjay_iid_t iid = 0;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    ASSERT_OK(_anjay_dm_read_resource_objlnk(
            anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 42, 1), &oid, &iid));
    ANJAY_MUTEX_UNLOCK(anjay);
    ASSERT_EQ(oid, 123);
    ASSERT_EQ(iid, 456);

    DM_TEST_FINISH;
}

#ifdef ANJAY_WITH_LWM2M11
AVS_UNIT_TEST(dm_res_read, u32_array_missing) {
    DM_TEST_INIT;

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 42, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 1, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_ABSENT },
                                                  ANJAY_MOCK_DM_RES_END });

    uint32_t *values;
    size_t num_values;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    ASSERT_EQ(_anjay_dm_read_resource_u32_array(
                      anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 42, 1),
                      &values, &num_values),
              ANJAY_ERR_NOT_FOUND);
    ANJAY_MUTEX_UNLOCK(anjay);

    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_res_read, u32_array_empty) {
    DM_TEST_INIT;

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 42, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 1, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 42, 1, 0, (const anjay_riid_t[]) { ANJAY_ID_INVALID });

    uint32_t *values;
    size_t num_values;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    ASSERT_OK(_anjay_dm_read_resource_u32_array(
            anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 42, 1), &values,
            &num_values));
    ANJAY_MUTEX_UNLOCK(anjay);
    ASSERT_EQ(num_values, 0);
    avs_free(values);

    DM_TEST_FINISH;
}

AVS_UNIT_TEST(dm_res_read, u64_array_multiple_elements) {
    DM_TEST_INIT;

    _anjay_mock_dm_expect_list_resources(
            anjay, &OBJ, 42, 0,
            (const anjay_mock_dm_res_entry_t[]) { { 1, ANJAY_DM_RES_RM,
                                                    ANJAY_DM_RES_PRESENT },
                                                  ANJAY_MOCK_DM_RES_END });
    _anjay_mock_dm_expect_list_resource_instances(
            anjay, &OBJ, 42, 1, 0,
            (const anjay_riid_t[]) { 12, 34, ANJAY_ID_INVALID });
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 42, 1, 12, 0,
                                        ANJAY_MOCK_DM_UINT(0, 111));
    _anjay_mock_dm_expect_resource_read(anjay, &OBJ, 42, 1, 34, 0,
                                        ANJAY_MOCK_DM_UINT(0, 222));

    uint32_t *values;
    size_t num_values;
    ANJAY_MUTEX_LOCK(anjay_unlocked, anjay);
    ASSERT_OK(_anjay_dm_read_resource_u32_array(
            anjay_unlocked, &MAKE_RESOURCE_PATH(OBJ->oid, 42, 1), &values,
            &num_values));
    ANJAY_MUTEX_UNLOCK(anjay);
    ASSERT_NOT_NULL(values);
    ASSERT_EQ(num_values, 2);
    ASSERT_EQ(values[0], 111);
    ASSERT_EQ(values[1], 222);
    avs_free(values);

    DM_TEST_FINISH;
}
#endif // ANJAY_WITH_LWM2M11
