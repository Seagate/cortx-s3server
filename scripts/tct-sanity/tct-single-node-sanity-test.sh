#!/bin/bash
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#


FS_ROOT=$1
TCT_TEST_DATA_DIR=$(mktemp -d --tmpdir=$FS_ROOT)
TEST_FILE="$TCT_TEST_DATA_DIR/48MB-data.$(hostname)"
TCT_CONSOLE_LOG="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/tct_stdout"

function cleanup {
    rm -rf $TCT_TEST_DATA_DIR
}
trap cleanup EXIT

touch $TCT_CONSOLE_LOG
echo "*** TCT Sanity test stout: $(hostname) ***" > $TCT_CONSOLE_LOG
echo "Create file: $TEST_FILE" >> $TCT_CONSOLE_LOG
dd if=/dev/urandom of=$TEST_FILE bs=16M count=3 status=none

TCT_TEST_RESULT=""


BEFORE_MD5=$(md5sum $TEST_FILE)
mmcloudgateway files migrate $TEST_FILE >> $TCT_CONSOLE_LOG 2>&1

mmcloudgateway files list $TEST_FILE 2>> $TCT_CONSOLE_LOG | grep -q 'Non-resident' &&
    TCT_TEST_RESULT="MIGRATION: OK" || TCT_TEST_RESULT="MIGRATION: FAILED"

mmcloudgateway files recall $TEST_FILE >> $TCT_CONSOLE_LOG 2>&1

mmcloudgateway files list $TEST_FILE 2>> $TCT_CONSOLE_LOG | grep -q 'Co-resident' &&
    TCT_TEST_RESULT="$TCT_TEST_RESULT\tRECALL: OK" ||
    TCT_TEST_RESULT="$TCT_TEST_RESULT\tRECALL: FAILED"

AFTER_MD5=$(md5sum $TEST_FILE)

[[ $BEFORE_MD5 == $AFTER_MD5 ]] &&
    TCT_TEST_RESULT="$TCT_TEST_RESULT\tDATA INTEGRITY: OK" ||
    TCT_TEST_RESULT="$TCT_TEST_RESULT\tDATA INTEGRITY: FAILED"

mmcloudgateway files delete --delete-local-file $TEST_FILE >> $TCT_CONSOLE_LOG 2>&1

mmcloudgateway files reconcile $FS_ROOT >> $TCT_CONSOLE_LOG 2>&1

echo -e "\n$TCT_TEST_RESULT" >> $TCT_CONSOLE_LOG
echo -e $TCT_TEST_RESULT
