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

set -xe

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
OS=$(cat /etc/os-release | grep -w ID | cut -d '=' -f 2)
VERSION=$(cat /etc/os-release | grep -w VERSION_ID | cut -d '=' -f 2)

GIT_VER=
S3TEST_SUITE_VERSION=1.0.0
VER_PATH_EXCL=0
PATH_SRC=""

usage() { echo "Usage: $0 (-G <git short revision> | -P <path to sources>) [-S <S3 Test suite version>]" 1>&2; exit 1; }

while getopts ":G:S:P:" o; do
    case "${o}" in
        G)
            GIT_VER=${OPTARG}
            [ -z "${GIT_VER}" ] && (echo "Git revision cannot be empty"; usage)
            [ $VER_PATH_EXCL == 1 ] && (echo "Only one of -G or -P can be specified"; usage)
            VER_PATH_EXCL=1
            ;;
        S)
            S3TEST_SUITE_VERSION=${OPTARG}
            ;;
        P)
            PATH_SRC=$(realpath ${OPTARG})
            [ -d "${PATH_SRC}" ] || (echo "Folder path does not exist"; usage)
            [ $VER_PATH_EXCL == 1 ] && (echo "Only one of -G or -P can be specified"; usage)
            VER_PATH_EXCL=1
            ;;

        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

[ $VER_PATH_EXCL == 0 ] && (echo "At least one of -G or -P should be specified"; usage)

echo "Using [S3TEST_SUITE_VERSION=${S3TEST_SUITE_VERSION}] ..."
! [ -z "${GIT_VER}" ] && echo "Using [GIT_VER=${GIT_VER}] ..."
! [ -z "${PATH_SRC}" ] && echo "Using [PATH_SRC=${PATH_SRC}] ..."

mkdir -p ~/rpmbuild/SOURCES/
cd ~/rpmbuild/SOURCES/
rm -rf cortx-s3-test*
if ! [ -z "${GIT_VER}" ]; then
  mkdir -p cortx-s3-test-${S3TEST_SUITE_VERSION}-git${GIT_VER}
  cd cortx-s3-test-${S3TEST_SUITE_VERSION}-git${GIT_VER}
  # For sake of test, attempt checkout of version
  git checkout ${GIT_VER}
elif ! [ -z "${PATH_SRC}" ]; then
    GIT_VER=$(git --git-dir "${PATH_SRC}"/.git rev-parse --short HEAD)
    mkdir -p cortx-s3-test-${S3TEST_SUITE_VERSION}-git${GIT_VER}
    cd cortx-s3-test-${S3TEST_SUITE_VERSION}-git${GIT_VER}
fi

mkdir -p scripts
mkdir -p certs/stx-s3-clients/s3/

cp "${PATH_SRC}/scripts/s3-sanity/s3-sanity-test.sh" "scripts/s3-sanity-test.sh"
cp "${PATH_SRC}/.s3cfg" .
cp "${PATH_SRC}/ansible/files/certs/stx-s3-clients/s3/ca.crt" "certs/stx-s3-clients/s3/ca.crt"
cd ~/rpmbuild/SOURCES/
tar -zcvf cortx-s3-test-${S3TEST_SUITE_VERSION}-git${GIT_VER}.tar.gz cortx-s3-test-${S3TEST_SUITE_VERSION}-git${GIT_VER}

rm -rf cortx-s3-test-${S3TEST_SUITE_VERSION}-git${GIT_VER}

extra_defines=()
if [ $VERSION = "\"8.0\"" ]; then
  extra_defines=(--define "s3_with_python36_ver8 1")
fi
yum-builddep -y ${BASEDIR}/s3testsuite.spec "${extra_defines[@]}"
rpmbuild -ba --define "_s3_test_version ${S3TEST_SUITE_VERSION}"  --define "_s3_test_git_ver git${GIT_VER}" "${extra_defines[@]}" ${BASEDIR}/s3testsuite.spec --with python3
