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


# This script installs virtual env and creates a virtual env for python3.6
# in the user's current directory
#
# It also installs all the dependencies required for pyclient.

pip install virtualenv
virtualenv -p /usr/local/bin/python3.6 motr_st
source motr_st/bin/activate
pip3 install python-dateutil==2.4.2
pip3 install pyyaml==3.11
pip3 install xmltodict==0.9.2
pip3 install boto3==1.2.2
pip3 install botocore==1.3.8

echo ""
echo ""
echo "Add the following to /etc/hosts"
echo "127.0.0.1 iam.seagate.com sts.seagate.com"
echo "127.0.0.1 s3.seagate.com"
