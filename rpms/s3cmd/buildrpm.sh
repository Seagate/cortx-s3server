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
OS_VERSION=$(cat /etc/os-release | grep -w VERSION_ID | cut -d '=' -f 2)
SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
# previous build version was 2.0.2 and commit version was 4801552f441cf12dec53099a6abc2b8aa36ccca4
VERSION=1.6.1
COMMIT_VER=4c2489361d353db1a1815172a6143c8f5a2d1c40
SHORT_COMMIT_VER=4c24893

cd ~/rpmbuild/SOURCES/
rm -rf s3cmd*

git clone -b v${VERSION} http://github.com/s3tools/s3cmd s3cmd-${VERSION}-${SHORT_COMMIT_VER}
cd s3cmd-${VERSION}-${SHORT_COMMIT_VER}
git checkout 4c2489361d353db1a1815172a6143c8f5a2d1c40 -f
cd ~/rpmbuild/SOURCES/
tar -zcvf s3cmd-${VERSION}-${SHORT_COMMIT_VER}.tar.gz s3cmd-${VERSION}-${SHORT_COMMIT_VER}
rm -rf s3cmd-${VERSION}-${SHORT_COMMIT_VER}

cp ${BASEDIR}/s3cmd_${VERSION}_max_retries.patch .

if [ "$OS_VERSION" = "\"8.0\"" ]; then
  yum-builddep -y ${BASEDIR}/cortx-s3-s3cmd.spec --define 's3_with_python36_ver8 1'
  rpmbuild -ba ${BASEDIR}/cortx-s3-s3cmd.spec --define 's3_with_python36_ver8 1'
else
  yum-builddep -y ${BASEDIR}/cortx-s3-s3cmd.spec --define 's3_with_python36 1'
  rpmbuild -ba ${BASEDIR}/cortx-s3-s3cmd.spec --define 's3_with_python36 1'
fi
