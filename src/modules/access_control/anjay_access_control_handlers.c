/*
 * Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
 * AVSystem Anjay LwM2M SDK
 * All rights reserved.
 *
 * Licensed under the AVSystem-5-clause License.
 * See the attached LICENSE file for details.
 */

#include <anjay_init.h>

#ifdef ANJAY_WITH_MODULE_ACCESS_CONTROL

#    include <inttypes.h>
#    include <string.h>

#    include <anjay_modules/anjay_access_utils.h>
#    include <anjay_modules/anjay_notify.h>
#    include <avsystem/commons/avs_sorted_set.h>

#    include "anjay_mod_access_control.h"

VISIBILITY_SOURCE_BEGIN

static access_control_instance_t *
find_instance(access_control_t *access_control, anjay_iid_t iid) {
    if (!access_control->last_accessed_instance
            || access_control->last_accessed_instance->iid != iid) {
        access_control->last_accessed_instance = NULL;
        access_control_instance_t *it;
        AVS_LIST_FOREACH(it, access_control->current.instances) {
            if (it->iid >= iid) {
                if (it->iid == iid) {
                    access_control->last_accessed_instance = it;
                }
                break;
            }
        }
    }
    return access_control->last_accessed_instance;
}

static int ac_list_instances(anjay_unlocked_t *anjay,
                             obj_ptr_t obj_ptr,
                             anjay_unlocked_dm_list_ctx_t *ctx) {
    (void) anjay;
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    if (!access_control) {
        return ANJAY_ERR_INTERNAL;
    }
    AVS_LIST(access_control_instance_t) it;
    AVS_LIST_FOREACH(it, access_control->current.instances) {
        _anjay_dm_emit_unlocked(ctx, it->iid);
    }
    return 0;
}

static int
ac_instance_reset(anjay_unlocked_t *anjay, obj_ptr_t obj_ptr, anjay_iid_t iid) {
    (void) anjay;
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    access_control_instance_t *inst = find_instance(access_control, iid);
    if (!inst) {
        return ANJAY_ERR_NOT_FOUND;
    }
    AVS_LIST_CLEAR(&inst->acl);
    inst->has_acl = false;
    inst->owner = 0;
    access_control->needs_validation = true;
    _anjay_access_control_mark_modified(access_control);
    return 0;
}

static int ac_instance_create(anjay_unlocked_t *anjay,
                              const anjay_dm_installed_object_t obj_ptr,
                              anjay_iid_t iid) {
    (void) anjay;
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    AVS_LIST(access_control_instance_t) new_instance =
            AVS_LIST_NEW_ELEMENT(access_control_instance_t);
    if (!new_instance) {
        ac_log(ERROR, _("out of memory"));
        return ANJAY_ERR_INTERNAL;
    }
    *new_instance = (access_control_instance_t) {
        .iid = iid,
        .target = {
            .oid = 0,
            .iid = -1
        },
        .owner = ANJAY_SSID_BOOTSTRAP,
        .has_acl = false,
        .acl = NULL
    };
    int retval = _anjay_access_control_add_instance(access_control,
                                                    new_instance, NULL);
    if (retval) {
        AVS_LIST_CLEAR(&new_instance);
    }
    access_control->needs_validation = true;
    _anjay_access_control_mark_modified(access_control);
    return retval;
}

static int ac_instance_remove(anjay_unlocked_t *anjay,
                              const anjay_dm_installed_object_t obj_ptr,
                              anjay_iid_t iid) {
    (void) anjay;
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    AVS_LIST(access_control_instance_t) *it;
    AVS_LIST_FOREACH_PTR(it, &access_control->current.instances) {
        if ((*it)->iid == iid) {
            if (access_control->last_accessed_instance
                    && access_control->last_accessed_instance->iid == iid) {
                access_control->last_accessed_instance = NULL;
            }
            AVS_LIST_CLEAR(&(*it)->acl);
            AVS_LIST_DELETE(it);
            _anjay_access_control_mark_modified(access_control);
            return 0;
        } else if ((*it)->iid > iid) {
            break;
        }
    }
    return ANJAY_ERR_NOT_FOUND;
}

static int ac_list_resources(anjay_unlocked_t *anjay,
                             obj_ptr_t obj_ptr,
                             anjay_iid_t iid,
                             anjay_unlocked_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    access_control_instance_t *inst =
            find_instance(_anjay_access_control_from_obj_ptr(obj_ptr), iid);

    _anjay_dm_emit_res_unlocked(ctx, ANJAY_DM_RID_ACCESS_CONTROL_OID,
                                ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    _anjay_dm_emit_res_unlocked(ctx, ANJAY_DM_RID_ACCESS_CONTROL_OIID,
                                ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    _anjay_dm_emit_res_unlocked(ctx, ANJAY_DM_RID_ACCESS_CONTROL_ACL,
                                ANJAY_DM_RES_RWM,
                                (inst && inst->has_acl) ? ANJAY_DM_RES_PRESENT
                                                        : ANJAY_DM_RES_ABSENT);
    _anjay_dm_emit_res_unlocked(ctx, ANJAY_DM_RID_ACCESS_CONTROL_OWNER,
                                ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    return 0;
}

static int ac_resource_read(anjay_unlocked_t *anjay,
                            obj_ptr_t obj_ptr,
                            anjay_iid_t iid,
                            anjay_rid_t rid,
                            anjay_riid_t riid,
                            anjay_unlocked_output_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    access_control_instance_t *inst =
            find_instance(_anjay_access_control_from_obj_ptr(obj_ptr), iid);
    if (!inst) {
        return ANJAY_ERR_NOT_FOUND;
    }

    switch (rid) {
    case ANJAY_DM_RID_ACCESS_CONTROL_OID:
        assert(riid == ANJAY_ID_INVALID);
        return _anjay_ret_i64_unlocked(ctx, (int32_t) inst->target.oid);
    case ANJAY_DM_RID_ACCESS_CONTROL_OIID:
        assert(riid == ANJAY_ID_INVALID);
        return _anjay_ret_i64_unlocked(ctx, (int32_t) inst->target.iid);
    case ANJAY_DM_RID_ACCESS_CONTROL_ACL: {
        acl_entry_t *it;
        AVS_LIST_FOREACH(it, inst->acl) {
            if (it->ssid >= riid) {
                break;
            }
        }
        if (!it || it->ssid != riid) {
            return ANJAY_ERR_NOT_FOUND;
        }
        return _anjay_ret_i64_unlocked(ctx, it->mask);
    }
    case ANJAY_DM_RID_ACCESS_CONTROL_OWNER:
        assert(riid == ANJAY_ID_INVALID);
        return _anjay_ret_i64_unlocked(ctx, (int32_t) inst->owner);
    default:
        AVS_UNREACHABLE("Read called on unknown Access Control resource");
        return ANJAY_ERR_NOT_IMPLEMENTED;
    }
}

static int write_to_acl_array(AVS_LIST(acl_entry_t) *acl,
                              anjay_ssid_t ssid,
                              anjay_unlocked_input_ctx_t *ctx) {
    int32_t mask;
    if (_anjay_get_i32_unlocked(ctx, &mask)) {
        return ANJAY_ERR_INTERNAL;
    }
    AVS_LIST(acl_entry_t) *it;
    AVS_LIST_FOREACH_PTR(it, acl) {
        if ((*it)->ssid >= ssid) {
            if ((*it)->ssid == ssid) {
                (*it)->mask = (anjay_access_mask_t) mask;
            }
            break;
        }
    }

    if (!*it || (*it)->ssid != ssid) {
        AVS_LIST(acl_entry_t) new_entry = AVS_LIST_NEW_ELEMENT(acl_entry_t);
        if (!new_entry) {
            return ANJAY_ERR_INTERNAL;
        }
        *new_entry = (acl_entry_t) {
            .ssid = ssid,
            .mask = (anjay_access_mask_t) mask
        };
        AVS_LIST_INSERT(it, new_entry);
    }
    return 0;
}

static int ac_resource_write(anjay_unlocked_t *anjay,
                             obj_ptr_t obj_ptr,
                             anjay_iid_t iid,
                             anjay_rid_t rid,
                             anjay_riid_t riid,
                             anjay_unlocked_input_ctx_t *ctx) {
    (void) anjay;
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    access_control_instance_t *inst = find_instance(access_control, iid);
    if (!inst) {
        return ANJAY_ERR_NOT_FOUND;
    }

    switch (rid) {
    case ANJAY_DM_RID_ACCESS_CONTROL_OID: {
        assert(riid == ANJAY_ID_INVALID);
        int32_t oid;
        int retval = _anjay_get_i32_unlocked(ctx, &oid);
        if (retval) {
            return retval;
        } else if (!_anjay_access_control_target_oid_valid(oid)) {
            return ANJAY_ERR_BAD_REQUEST;
        }
        inst->target.oid = (anjay_oid_t) oid;
        access_control->needs_validation = true;
        _anjay_access_control_mark_modified(access_control);
        return 0;
    }
    case ANJAY_DM_RID_ACCESS_CONTROL_OIID: {
        assert(riid == ANJAY_ID_INVALID);
        int32_t oiid;
        int retval = _anjay_get_i32_unlocked(ctx, &oiid);
        if (retval) {
            return retval;
        } else if (oiid < 0 || oiid > UINT16_MAX) {
            return ANJAY_ERR_BAD_REQUEST;
        }
        inst->target.iid = (anjay_iid_t) oiid;
        access_control->needs_validation = true;
        _anjay_access_control_mark_modified(access_control);
        return 0;
    }
    case ANJAY_DM_RID_ACCESS_CONTROL_ACL: {
        int retval = write_to_acl_array(&inst->acl, riid, ctx);
        if (!retval) {
            inst->has_acl = true;
            access_control->needs_validation = true;
            _anjay_access_control_mark_modified(access_control);
        }
        return retval;
    }
    case ANJAY_DM_RID_ACCESS_CONTROL_OWNER: {
        assert(riid == ANJAY_ID_INVALID);
        int32_t ssid;
        int retval = _anjay_get_i32_unlocked(ctx, &ssid);
        if (retval) {
            return retval;
        } else if (ssid <= 0 || ssid > ANJAY_SSID_BOOTSTRAP) {
            return ANJAY_ERR_BAD_REQUEST;
        }
        inst->owner = (anjay_ssid_t) ssid;
        access_control->needs_validation = true;
        _anjay_access_control_mark_modified(access_control);
        return 0;
    }
    default:
        AVS_UNREACHABLE("Write called on unknown Access Control resource");
        return ANJAY_ERR_NOT_FOUND;
    }
}

static int ac_resource_reset(anjay_unlocked_t *anjay,
                             const anjay_dm_installed_object_t obj_ptr,
                             anjay_iid_t iid,
                             anjay_rid_t rid) {
    (void) anjay;
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    access_control_instance_t *inst = find_instance(access_control, iid);
    if (!inst) {
        return ANJAY_ERR_NOT_FOUND;
    }

    assert(rid == ANJAY_DM_RID_ACCESS_CONTROL_ACL);
    (void) rid;
    AVS_LIST_CLEAR(&inst->acl);
    inst->has_acl = true;
    access_control->needs_validation = true;
    _anjay_access_control_mark_modified(access_control);
    return 0;
}

static int ac_list_resource_instances(anjay_unlocked_t *anjay,
                                      obj_ptr_t obj_ptr,
                                      anjay_iid_t iid,
                                      anjay_rid_t rid,
                                      anjay_unlocked_dm_list_ctx_t *ctx) {
    (void) anjay;
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    access_control_instance_t *inst = find_instance(access_control, iid);
    if (!inst) {
        return ANJAY_ERR_NOT_FOUND;
    }

    switch (rid) {
    case ANJAY_DM_RID_ACCESS_CONTROL_ACL: {
        acl_entry_t *it;
        AVS_LIST_FOREACH(it, inst->acl) {
            _anjay_dm_emit_unlocked(ctx, it->ssid);
        }
        return 0;
    }
    default:
        AVS_UNREACHABLE(
                "Attempted to list instances in a single-instance resource");
        return ANJAY_ERR_INTERNAL;
    }
}

static int ac_transaction_begin(anjay_unlocked_t *anjay, obj_ptr_t obj_ptr) {
    (void) anjay;
    access_control_t *ac = _anjay_access_control_from_obj_ptr(obj_ptr);
    assert(!ac->in_transaction);
    if (_anjay_access_control_clone_state(&ac->saved_state, &ac->current)) {
        ac_log(ERROR, _("out of memory"));
        return ANJAY_ERR_INTERNAL;
    }
    ac->in_transaction = true;
    return 0;
}

static int anjay_ssid_cmp(const void *left, const void *right) {
    return *(const anjay_ssid_t *) left - *(const anjay_ssid_t *) right;
}

static int add_ssid(AVS_SORTED_SET(anjay_ssid_t) ssids_list,
                    anjay_ssid_t ssid) {
    // here it is actually more likely for the SSID to be already present
    // so we use find-then-insert logic to avoid unnecessary allocations
    AVS_SORTED_SET_ELEM(anjay_ssid_t) elem =
            AVS_SORTED_SET_FIND(ssids_list, &ssid);
    if (!elem) {
        if (!(elem = AVS_SORTED_SET_ELEM_NEW(anjay_ssid_t))) {
            ac_log(ERROR, _("out of memory"));
            return -1;
        }
        *elem = ssid;
        if (AVS_SORTED_SET_INSERT(ssids_list, elem) != elem) {
            AVS_UNREACHABLE("Internal error: cannot add tree element");
        }
    }
    return 0;
}

/**
 * Validates that <c>ssid</c> can be used as a key (RIID) in the ACL - it needs
 * to either reference a valid server, or be equal to @ref ANJAY_SSID_ANY (0).
 */
int _anjay_access_control_validate_ssid(anjay_unlocked_t *anjay,
                                        anjay_ssid_t ssid) {
    return (ssid != ANJAY_SSID_BOOTSTRAP
            && (ssid == ANJAY_SSID_ANY || _anjay_dm_ssid_exists(anjay, ssid)))
                   ? 0
                   : -1;
}

static int ac_transaction_validate(anjay_unlocked_t *anjay, obj_ptr_t obj_ptr) {
    access_control_t *access_control =
            _anjay_access_control_from_obj_ptr(obj_ptr);
    assert(access_control->in_transaction);
    int result = 0;
    anjay_acl_ref_validation_ctx_t validation_ctx =
            _anjay_acl_ref_validation_ctx_new();
    AVS_SORTED_SET(anjay_ssid_t) ssids_used = NULL;
    if (access_control->needs_validation) {
        if (!(ssids_used = AVS_SORTED_SET_NEW(anjay_ssid_t, anjay_ssid_cmp))) {
            ac_log(ERROR, _("out of memory"));
            goto finish;
        }
        access_control_instance_t *inst;
        result = ANJAY_ERR_BAD_REQUEST;
        AVS_LIST_FOREACH(inst, access_control->current.instances) {
            if (!_anjay_access_control_target_oid_valid(inst->target.oid)
                    || !_anjay_access_control_target_iid_valid(inst->target.iid)
                    || _anjay_acl_ref_validate_inst_ref(
                               anjay, &validation_ctx, inst->target.oid,
                               (anjay_iid_t) inst->target.iid)
                    || (inst->owner != ANJAY_SSID_BOOTSTRAP
                        && add_ssid(ssids_used, inst->owner))) {
                ac_log(WARNING,
                       _("Validation failed for target: ") "/%" PRIu16
                                                           "/%" PRId32,
                       inst->target.oid, inst->target.iid);
                goto finish;
            }
            acl_entry_t *acl;
            AVS_LIST_FOREACH(acl, inst->acl) {
                if (add_ssid(ssids_used, acl->ssid)) {
                    goto finish;
                }
            }
        }
        AVS_SORTED_SET_DELETE(&ssids_used) {
            if (_anjay_access_control_validate_ssid(anjay, **ssids_used)) {
                ac_log(WARNING,
                       _("Validation failed: invalid SSID: ") "%" PRIu16,
                       **ssids_used);
                goto finish;
            }
        }
        result = 0;
        access_control->needs_validation = false;
    }
finish:
    _anjay_acl_ref_validation_ctx_cleanup(&validation_ctx);
    AVS_SORTED_SET_DELETE(&ssids_used);
    return result;
}

static int ac_transaction_commit(anjay_unlocked_t *anjay, obj_ptr_t obj_ptr) {
    (void) anjay;
    access_control_t *ac = _anjay_access_control_from_obj_ptr(obj_ptr);
    assert(ac->in_transaction);
    _anjay_access_control_clear_state(&ac->saved_state);
    ac->needs_validation = false;
    ac->in_transaction = false;
    return 0;
}

static int ac_transaction_rollback(anjay_unlocked_t *anjay, obj_ptr_t obj_ptr) {
    (void) anjay;
    access_control_t *ac = _anjay_access_control_from_obj_ptr(obj_ptr);
    assert(ac->in_transaction);
    _anjay_access_control_clear_state(&ac->current);
    ac->current = ac->saved_state;
    memset(&ac->saved_state, 0, sizeof(ac->saved_state));
    ac->needs_validation = false;
    ac->in_transaction = false;
    ac->last_accessed_instance = NULL;
    return 0;
}

static void ac_delete(void *access_control_) {
    access_control_t *access_control = (access_control_t *) access_control_;
    _anjay_access_control_clear_state(&access_control->current);
    _anjay_access_control_clear_state(&access_control->saved_state);
    // NOTE: access_control itself will be freed when cleaning the objects list
}

void anjay_access_control_purge(anjay_t *anjay_locked) {
    assert(anjay_locked);
    ANJAY_MUTEX_LOCK(anjay, anjay_locked);
    access_control_t *ac = _anjay_access_control_get(anjay);
    if (!ac) {
        ac_log(ERROR, _("Access Control object is not registered"));
    } else {
        _anjay_access_control_clear_state(&ac->current);
        _anjay_access_control_mark_modified(ac);
        ac->last_accessed_instance = NULL;
        ac->needs_validation = false;
        if (_anjay_notify_instances_changed_unlocked(
                    anjay, ANJAY_DM_OID_ACCESS_CONTROL)) {
            ac_log(WARNING, _("Could not schedule access control instance "
                              "changes notifications"));
        }
    }
    ANJAY_MUTEX_UNLOCK(anjay_locked);
}

bool anjay_access_control_is_modified(anjay_t *anjay_locked) {
    assert(anjay_locked);
    bool result = false;
    ANJAY_MUTEX_LOCK(anjay, anjay_locked);
    access_control_t *ac = _anjay_access_control_get(anjay);
    if (!ac) {
        ac_log(ERROR, _("Access Control object is not registered"));
    } else if (ac->in_transaction) {
        result = ac->saved_state.modified_since_persist;
    } else {
        result = ac->current.modified_since_persist;
    }
    ANJAY_MUTEX_UNLOCK(anjay_locked);
    return result;
}

static const anjay_dm_module_t ACCESS_CONTROL_MODULE = {
    .deleter = ac_delete
};

static const anjay_unlocked_dm_object_def_t ACCESS_CONTROL = {
    .oid = ANJAY_DM_OID_ACCESS_CONTROL,
    .handlers = {
        .list_instances = ac_list_instances,
        .instance_reset = ac_instance_reset,
        .instance_create = ac_instance_create,
        .instance_remove = ac_instance_remove,
        .list_resources = ac_list_resources,
        .resource_read = ac_resource_read,
        .resource_write = ac_resource_write,
        .resource_reset = ac_resource_reset,
        .list_resource_instances = ac_list_resource_instances,
        .transaction_begin = ac_transaction_begin,
        .transaction_validate = ac_transaction_validate,
        .transaction_commit = ac_transaction_commit,
        .transaction_rollback = ac_transaction_rollback
    }
};

int anjay_access_control_install(anjay_t *anjay_locked) {
    if (!anjay_locked) {
        ac_log(ERROR, _("ANJAY object must not be NULL"));
        return -1;
    }
    int result = -1;
    ANJAY_MUTEX_LOCK(anjay, anjay_locked);
    AVS_LIST(access_control_t) access_control =
            AVS_LIST_NEW_ELEMENT(access_control_t);
    if (access_control) {
        access_control->obj_def = &ACCESS_CONTROL;
        _anjay_dm_installed_object_init_unlocked(&access_control->obj_def_ptr,
                                                 &access_control->obj_def);
        if (!_anjay_dm_module_install(anjay, &ACCESS_CONTROL_MODULE,
                                      access_control)) {
            AVS_STATIC_ASSERT(offsetof(access_control_t, obj_def_ptr) == 0,
                              obj_def_ptr_is_first_field);
            AVS_LIST(anjay_dm_installed_object_t) entry =
                    &access_control->obj_def_ptr;
            if (_anjay_register_object_unlocked(anjay, &entry)) {
                result = _anjay_dm_module_uninstall(anjay,
                                                    &ACCESS_CONTROL_MODULE);
                assert(!result);
                result = -1;
            } else {
                result = 0;
            }
        }
        if (result) {
            AVS_LIST_CLEAR(&access_control);
        }
    }
    ANJAY_MUTEX_UNLOCK(anjay_locked);
    return result;
}

access_control_t *_anjay_access_control_get(anjay_unlocked_t *anjay) {
    return (access_control_t *) _anjay_dm_module_get_arg(
            anjay, &ACCESS_CONTROL_MODULE);
}

#endif // ANJAY_WITH_MODULE_ACCESS_CONTROL
