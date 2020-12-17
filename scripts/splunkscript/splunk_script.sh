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



SPLUNK_DIR="/root/splunk/s3-tests"
SPLUNK_INIPATH="$SPLUNK_DIR/splunk.conf"
SPLUNK_TEST_DATA_V2="/root/cortx-s3server/scripts/splunkscript/splunktestfilev2.txt"
SPLUNK_TEST_DATA_V4="/root/cortx-s3server/scripts/splunkscript/splunktestfilev4.txt"


# Set up Virtualenv
echo "starting virtualenv"
source $SPLUNK_DIR/virtualenv/bin/activate

# Export config filepath
export S3TEST_CONF=$SPLUNK_INIPATH

# Running splunk with aws v2signature
export S3_USE_SIGV4=0

# Log Directory
splunk_log_dir="/root/logs/splunk-log"
mkdir -p "$splunk_log_dir"

while read test_name; do
      $SPLUNK_DIR/virtualenv/bin/nosetests $test_name >> $splunk_log_dir/splunk.log 2>&1
done < $SPLUNK_TEST_DATA_V2

echo "sleeptime30s"
sleep 30s

# Running splunk with aws v4 signature

export S3_USE_SIGV4=1

while read test_name; do
        $SPLUNK_DIR/virtualenv/bin/nosetests $test_name >> $splunk_log_dir/splunkv4.log 2>&1
done < $SPLUNK_TEST_DATA_V4

export S3_USE_SIGV4=0

# Disabling virtual env
deactivate

echo "Splunk Suite Ran Successfully!!!"
