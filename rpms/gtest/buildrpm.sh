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


set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.10.0

cd ~/rpmbuild/SOURCES/
rm -rf gtest* googletest*

wget https://github.com/google/googletest/archive/release-${VERSION}.tar.gz

tar -xvzf release-${VERSION}.tar.gz
rm -f release-${VERSION}.tar.gz
mv googletest-release-1.10.0 gtest-${VERSION}
tar -cvzf gtest-${VERSION}.tar.gz gtest-${VERSION}
rm -rf gtest-${VERSION}

cd -

yum-builddep -y ${BASEDIR}/cortx-s3-gtest.spec
rpmbuild -ba ${BASEDIR}/cortx-s3-gtest.spec

