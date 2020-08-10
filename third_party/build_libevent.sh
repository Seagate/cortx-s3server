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
# Script to build libevent.
# git repo: https://github.com/libevent/libevent.git
# github repo: git clone https://github.com/Seagate/libevent.git
# previous tag: release-2.0.22-stable commit: c51b159cff9f5e86696f5b9a4c6f517276056258
# previous patch: libevent-release-2.0.22-stable.patch
# Current tag: release-2.1.10-stable commit: 64a25bcdf54a3fc8001e76ea51e0506320567e17

cd libevent

#previous libevent patch was libevent-release-2.0.22-stable.patch

# Apply the current libevent patch for memory pool support
patch -f -p1 < ../../patches/libevent-release-2.1.10-stable.patch

INSTALL_DIR=`pwd`/s3_dist
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

./autogen.sh
./configure --prefix=$INSTALL_DIR

make clean
make
#make verify
make install

cd ..
