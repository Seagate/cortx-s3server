#!/bin/sh
set +e

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


CEPH_DIR="/root/s3-tests"
CEPH_INIPATH="$CEPH_DIR/seagates3conf.ini"
CEPH_TEST_DATA_V2="/root/cortx-s3server/scripts/cephscript/cephtestfilev2.txt"
CEPH_TEST_DATA_V4="/root/cortx-s3server/scripts/cephscript/cephtestfilev4.txt"


# Set up Virtualenv
echo "starting virtualenv"
source $CEPH_DIR/virtualenv/bin/activate

# Export config filepath
export S3TEST_CONF=$CEPH_INIPATH

# Running ceph test with aws v2 signature

export S3_USE_SIGV4=0

# Log Directory
ceph_log_dir="/root/logs/ceph-log"
mkdir -p "$ceph_log_dir"

while read test_name; do
        $CEPH_DIR/virtualenv/bin/nosetests $test_name >> $ceph_log_dir/cephrunv2.log 2>&1
done < $CEPH_TEST_DATA_V2

echo "sleeptime:30s"
sleep 30s

# Running ceph test with aws v4 signature

export S3_USE_SIGV4=1

while read test_name; do
        $CEPH_DIR/virtualenv/bin/nosetests $test_name >> $ceph_log_dir/cephrunv4.log 2>&1
done < $CEPH_TEST_DATA_V4

# Disabling virtual env
deactivate

echo "Ran CephSuite Successfully!!"
