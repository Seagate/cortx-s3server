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
# Script to build gmock.
# git repo: https://github.com/google/googlemock.git
# tag: release-1.7.0 commit: c440c8fafc6f60301197720617ce64028e09c79d

cd googlemock

rm -f gtest
ln -s ../googletest gtest

rm -rf build/
mkdir build

cd build
CFLAGS=-fPIC CXXFLAGS=-fPIC cmake ..
make

cd ..
cd ..
