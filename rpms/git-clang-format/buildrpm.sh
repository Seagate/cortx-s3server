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

VERSION=6.0
GIT_RELEASE_BRANCH=release_60

cd ~/rpmbuild/SOURCES/
rm -rf git-clang-format*

mkdir git-clang-format-${VERSION}

wget -O git-clang-format-${VERSION}/git-clang-format https://raw.githubusercontent.com/llvm-mirror/clang/${GIT_RELEASE_BRANCH}/tools/clang-format/git-clang-format
wget -O git-clang-format-${VERSION}/LICENSE.TXT https://raw.githubusercontent.com/llvm-mirror/clang/${GIT_RELEASE_BRANCH}/LICENSE.TXT

tar -zcvf git-clang-format-${VERSION}.tar.gz git-clang-format-${VERSION}
rm -rf git-clang-format-${VERSION}

cd -
yum-builddep -y ${BASEDIR}/cortx-s3-git-clang-format.spec
rpmbuild -ba ${BASEDIR}/cortx-s3-git-clang-format.spec

