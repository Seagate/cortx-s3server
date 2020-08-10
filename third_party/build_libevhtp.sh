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
# Script to build libevhtp.
# git repo: https://github.com/ellzey/libevhtp.git
# github repo: git clone https://github.com/Seagate/libevhtp.git
# previous branch: develop commit: a89d9b3f9fdf2ebef41893b3d5e4466f4b0ecfda
# previous patch: libevhtp-v1.2.11-prev.patch
# current branch: develop commit: c84f68d258d07c4015820ceb87fd17decd054bfc

VERSION=`cat /etc/os-release | sed -n 's/VERSION_ID=\"\([0-9].*\)\"/\1/p'`
major_version=`echo ${VERSION} | awk -F '.' '{ print $1 }'`

cd libevhtp
if [ "$major_version" = "8" ]; then
  # Apply the libevhtp rhel patch
  patch -f -p1 < ../../patches/libevhtp-v1.2.11-rhel8.patch
else
  # Apply the libevhtp patch
  patch -f -p1 < ../../patches/libevhtp-v1.2.11.patch
fi

INSTALL_DIR=`pwd`/s3_dist
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

# Note: googletest regex clashes with our regex. Disable regex by default

CFLAGS='-fPIC -DEVHTP_DISABLE_EVTHR' cmake . -DEVHTP_DISABLE_REGEX=ON -DCMAKE_PREFIX_PATH=`pwd`/../libevent/s3_dist -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make clean
make
make install

cd ..
