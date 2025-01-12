#!/usr/bin/env bash
#
# Copyright 2017-2022 AVSystem <avsystem@avsystem.com>
# AVSystem Anjay LwM2M SDK
# All rights reserved.
#
# Licensed under the AVSystem-5-clause License.
# See the attached LICENSE file for details.

set -e
. "$(dirname "$0")/../../tools/utils.sh"
SCRIPT_DIR="$(dirname "$(canonicalize "$0")")"
OBJECT_REGISTRY_SCRIPT="$SCRIPT_DIR/../../tools/lwm2m_object_registry.py"
CODEGEN_SCRIPT="$SCRIPT_DIR/../../tools/anjay_codegen.py"
TEMP_DIR="$(mktemp -d)"
atexit "rm -r $TEMP_DIR"

if [ "$1" == "--c++" ]; then
    CODEGEN_ARGS="$1"
    shift
fi
OIDS="$@"

log "running object registry test for objects: $OIDS"
log "code generator additional args: \"$CODEGEN_ARGS\""

for OID in $OIDS; do
    OBJECT_DEFINITION_FILE="$TEMP_DIR/$OID-object-def.xml"
    OBJECT_REGISTRY_ERROR_FILE="$TEMP_DIR/$OID-object-err.log"
    CODEGEN_ERROR_FILE="$TEMP_DIR/$OID-codegen-err.log"

    check_object_registry_script_error() {
        OBJECT_REGISTRY_ERROR=`cat "$OBJECT_REGISTRY_ERROR_FILE"`
        # Ignore ValueError, it is not critical
        echo "$OBJECT_REGISTRY_ERROR" | tail -1 | grep --quiet "ValueError" ||
            die "Unexpected error while getting object $OID definition:\n$OBJECT_REGISTRY_ERROR"
    }
    "$OBJECT_REGISTRY_SCRIPT" -g "$OID" 1>"$OBJECT_DEFINITION_FILE" \
        2>"$OBJECT_REGISTRY_ERROR_FILE" || check_object_registry_script_error && continue

    check_codegen_script_error() {
        CODEGEN_ERROR=`cat "$CODEGEN_ERROR_FILE"`
        echo "$CODEGEN_ERROR" | tail -1 |
            # Ignore "multiple-instance executable resources" error
            grep --quiet "multiple-instance executable resources are not supported" ||
            die "Unexpected error while generating code for object $OID:\n$CODEGEN_ERROR"
    }
    "$CODEGEN_SCRIPT" $CODEGEN_ARGS -i "$OBJECT_DEFINITION_FILE" 1>/dev/null \
        2>"$CODEGEN_ERROR_FILE" || check_codegen_script_error
done
