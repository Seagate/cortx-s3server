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
# Script to build glog.
# git repo: https://github.com/google/glog.git
# tag: v0.3.4 commit: d8cb47f77d1c31779f3ff890e1a5748483778d6a

cd glog

INSTALL_DIR=`pwd`/s3_dist
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

GFLAGS_HDR_DIR=../gflags/s3_dist/include
GFLAGS_LIB_DIR=../gflags/s3_dist/lib

CXXFLAGS="-fPIC -I$GFLAGS_HDR_DIR" LDFLAGS="-L$GFLAGS_LIB_DIR" ./configure --prefix=$INSTALL_DIR

touch configure.ac aclocal.m4 configure Makefile.am Makefile.in
make clean
make
make install

cd ..
