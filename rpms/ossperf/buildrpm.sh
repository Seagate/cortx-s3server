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

VERSION=3.0

cd ~/rpmbuild/SOURCES/
rm -rf ossperf

mkdir ossperf-${VERSION}

git clone https://github.com/christianbaun/ossperf  ossperf-${VERSION}
cd ossperf-${VERSION}
git checkout 58eafade5ada0f98d7b34f2d41cfc673c8d7b301 -f
cd ..
tar -zcvf ossperf-${VERSION}.tar.gz ossperf-${VERSION}
rm -rf ossperf-${VERSION}
cp ${BASEDIR}/ossperf.patch .

cd ~/rpmbuild/SOURCES/

rpmbuild -ba ${BASEDIR}/cortx-s3-ossperf.spec

