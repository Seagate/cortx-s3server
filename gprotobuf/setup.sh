#!/bin/sh -x
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


rm -rf protobuf

git clone -b v3.0.0-alpha-3.1 https://github.com/google/protobuf.git

cd protobuf
INSTALL_DIR=/opt/seagate/cortx/s3/gprotobuf
mkdir $INSTALL_DIR

./autogen.sh
./configure --prefix=$INSTALL_DIR

make
make check
sudo make install
