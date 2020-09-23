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

VERSION=0.10.0

cd ~/rpmbuild/SOURCES/
rm -rf apache-log4cxx*

# Todo : log4cxx should be added to our github repo to clone here
git clone https://gitbox.apache.org/repos/asf/logging-log4cxx.git apache-log4cxx-${VERSION}
cd apache-log4cxx-${VERSION}
git checkout c60dda4770ce848403f475ab9fa6accd8173d2d8 -f

cd -
tar -jcvf apache-log4cxx-${VERSION}.tar.bz2 apache-log4cxx-${VERSION}
rm -rf apache-log4cxx-${VERSION}

cp ${BASEDIR}/apache-log4cxx-${VERSION}.patch .

rpmbuild -ba ${BASEDIR}/cortx-s3-apache-log4cxx.spec

