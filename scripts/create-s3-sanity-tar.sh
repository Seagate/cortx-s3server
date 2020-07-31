#!/bin/sh
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


####################################
# Script to generate s3-sanity tar #
####################################

set -e

USAGE="USAGE: bash $(basename "$0") [-h | --help]
Generate tar file which has all required dependencies to run S3 sanity test.
where:
    --help      display this help and exit

Generated tar will have follwoing files:
    s3-sanity-test
    ├── jcloud
    │   └── jcloudclient.jar
    ├── ldap
    │   ├── add_test_account.sh
    │   ├── delete_test_account.sh
    │   └── test_data.ldif
    ├── README
    └── s3-sanity-test.sh"


case "$1" in
    --help | -h )
        echo "$USAGE"
        exit 0
        ;;
esac

TARGET_DIR="s3-sanity-test"
SRC_ROOT="$(dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd ) " )"

mkdir -p $TARGET_DIR/jcloud
mkdir -p $TARGET_DIR/ldap
cp $SRC_ROOT/auth-utils/jcloudclient/target/jcloudclient.jar $TARGET_DIR/jcloud/
cp $SRC_ROOT/scripts/ldap/add_test_account.sh $TARGET_DIR/ldap/
cp $SRC_ROOT/scripts/ldap/delete_test_account.sh $TARGET_DIR/ldap/
cp $SRC_ROOT/scripts/ldap/test_data.ldif $TARGET_DIR/ldap/
cp $SRC_ROOT/scripts/s3-sanity/s3-sanity-test.sh $TARGET_DIR/
cp $SRC_ROOT/scripts/s3-sanity/README $TARGET_DIR/

# create tar file
tar -cf s3-sanity-test.tar $TARGET_DIR
echo "$TARGET_DIR.tar created successfully."

# remove dir
rm -rf $TARGET_DIR
