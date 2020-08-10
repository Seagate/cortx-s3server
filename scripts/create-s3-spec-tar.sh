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

# Script to generate tar containing S3 specs (S3 system tests scripts)
set -e

TAR_NAME=s3-specs
TARGET_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/$TAR_NAME"
SRC_ROOT="$(dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd ) " )"
TEST_DIR="$TARGET_DIR/st/clitests"

mkdir -p $TEST_DIR/cfg
mkdir -p $TEST_DIR/resources
mkdir -p $TEST_DIR/test_data

working_dir=$(pwd)

# copy jclient.jar
cp $SRC_ROOT/auth-utils/jclient/target/jclient.jar $TEST_DIR

# copy jcloudclient.jar
cp $SRC_ROOT/auth-utils/jcloudclient/target/jcloudclient.jar $TEST_DIR


# copy test framework and spec files
cd $SRC_ROOT/st/clitests
test_files=$(git ls-files)
for file in $test_files
do
    cp -f $file $TEST_DIR/$file
done

cd $working_dir
# create tar file
tar -cf s3-specs.tar $TAR_NAME

# remove dir
rm -rf $TARGET_DIR
