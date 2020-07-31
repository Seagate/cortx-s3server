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

# Script to generate tct sepcs tar
set -e

TARGET_DIR="tct-specs"
SRC_ROOT="$(dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd ) " )"
ST_DIR="$TARGET_DIR/st"
SANITY_TEST_DIR="$TARGET_DIR/sanity-test"

mkdir -p $ST_DIR
mkdir -p $SANITY_TEST_DIR

# copy tct-sannity test
cp -r "$SRC_ROOT/st/clitests/tct-sanity/" "$SANITY_TEST_DIR/"

# copy test framework and spec files
test_files="framework.py ldap_setup.py cloud_setup.py mmcloud.py mmcloud_spec.py \
    setup.sh fs_helpers.py cfg"
for file in $test_files
do
    cp -r "$SRC_ROOT/st/clitests/$file" "$ST_DIR/"
done

# create tar file
tar -cvf tct-specs.tar $TARGET_DIR

# remove dir
rm -rf $TARGET_DIR
