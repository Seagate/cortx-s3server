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


# This script installs virtual env and creates a virtual env for python3.5
# in the user's home directory (/root if this script is run as root user.)
#
# It also installs all the dependencies required for system tests.

pip install virtualenv
virtualenv -p /usr/local/bin/python3.5 motr_st
source motr_st/bin/activate
pip install python-dateutil==2.4.2
pip install pyyaml==3.11
pip install xmltodict==0.9.2
pip install boto3==1.2.2
pip install botocore==1.3.8
pip install scripttest==1.3

echo ""
echo "Add the following to /etc/hosts"
echo "127.0.0.1 seagatebucket.s3.seagate.com seagate-bucket.s3.seagate.com seagatebucket123.s3.seagate.com seagate.bucket.s3.seagate.com"
echo "127.0.0.1 s3-us-west-2.seagate.com seagatebucket.s3-us-west-2.seagate.com"
echo "127.0.0.1 iam.seagate.com sts.seagate.com"
