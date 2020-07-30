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

#!/bin/sh -xe
# Script to build gflags.
# git repo: https://github.com/gflags/gflags.git
# tag: v2.2.0 commit: f8a0efe03aa69b3336d8e228b37d4ccb17324b88

cd gflags

INSTALL_DIR=`pwd`/s3_dist
rm -rf build/ $INSTALL_DIR
mkdir $INSTALL_DIR
mkdir build && cd build

CFLAGS=-fPIC CXXFLAGS=-fPIC cmake .. -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make
make install

cd ../..
