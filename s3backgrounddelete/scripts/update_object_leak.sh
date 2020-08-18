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

# Script to add leaked objects to probable delete index-id.

aws s3 rb s3://mybucket --force
echo Creating new bucket "mybucket"
aws s3 mb s3://mybucket
for value in {1..2}
do
echo Creating test object
dd if=/dev/urandom of=test_object bs=1M count=1 iflag=fullblock
echo Adding object_$value to S3server
curl -sS --header "x-seagate-faultinjection: enable,always,motr_entity_delete_fail" -X PUT https://s3.seagate.com --cacert /etc/ssl/stx-s3-clients/s3/ca.crt
aws s3 cp test_object s3://mybucket/test_object
done
rm -rf test_object
curl -sS --header "x-seagate-faultinjection: disable,noop,motr_entity_delete_fail" -X PUT https://s3.seagate.com --cacert /etc/ssl/stx-s3-clients/s3/ca.crt
