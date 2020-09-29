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



This Readme document provides instructions on how to create rpm package for 3rd party dependencies of CORTX-S3Server
For most of the dependencies, the procedure is straight forward:

1) Goto the 3rd party directory under "rpms" directory.
2) Run ./buildrpm script. If it does not fail, you will find the created package under /root/rpmbuild/RPMS directory.


For some package(s), we may need to make sure certain steps have been performed before running the buildrpm script
for that particular package.


Bazel:
Currently we are using Version: 0.13.0.
To create bazel package, perform below steps:
1) Make sure the /tmp directory on your VM has atleast 4GB of free space.
2) Run scripts/env/dev/init.sh [-a]
3) Now your VM have right version of openjdk (1.8.0_242) installed.
4) Run the <cortx-s3server>/rpms/bazel/buildrpm.sh script for bazel.

s3cmd:
Currently we are using 1.6.1 version of s3cmd with our custom patch
To build s3cmd package, please make sure you have pytho2.7 version available on your development environment.
