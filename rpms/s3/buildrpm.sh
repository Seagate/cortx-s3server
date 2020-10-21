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

GIT_VER=
S3_VERSION=1.0.0
PATH_SRC=""
VER_PATH_EXCL=0
INSTALL_AFTER_BUILD=0
ENABLE_DEBUG_LOG=0
DISABLE_MOTR=""

usage() { echo "Usage: $0 [-S <S3 version>] [-i] (-G <git short revision> | -P <path to sources>)" \
               "(specify [-l] to enable debug level logging)" \
               "(specify [-a] to build s3 rpm autonomously without motr rpm dependency)" 1>&2; exit 1; }

while getopts ":G:S:P:ila" x; do
    case "${x}" in
        G)
            GIT_VER=${OPTARG}
            [ -z "${GIT_VER}" ] && (echo "Git revision cannot be empty"; usage)
            [ $VER_PATH_EXCL == 1 ] && (echo "Only one of -G or -P can be specified"; usage)
            VER_PATH_EXCL=1
            ;;
        S)
            S3_VERSION=${OPTARG}
            ;;
        P)
            PATH_SRC=$(realpath ${OPTARG})
            [ -d "${PATH_SRC}" ] || (echo "Folder path does not exist"; usage)
            [ $VER_PATH_EXCL == 1 ] && (echo "Only one of -G or -P can be specified"; usage)
            VER_PATH_EXCL=1
            ;;
        i)
            INSTALL_AFTER_BUILD=1
            ;;
        l)
            ENABLE_DEBUG_LOG=1;
            ;;
        a)
            DISABLE_MOTR="disable_cortxmotr_dependencies 1"
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

[ $VER_PATH_EXCL == 0 ] && (echo "At least one of -G or -P should be specified"; usage)

echo "Using [S3_VERSION=${S3_VERSION}] ..."
! [ -z "${GIT_VER}" ] && echo "Using [GIT_VER=${GIT_VER}] ..."
! [ -z "${PATH_SRC}" ] && echo "Using [PATH_SRC=${PATH_SRC}] ..."
echo "Install after build ${INSTALL_AFTER_BUILD}"

set -xe

mkdir -p ~/rpmbuild/SOURCES/
cd ~/rpmbuild/SOURCES/
rm -rf cortx-s3server*

if ! [ -z "${GIT_VER}" ]; then
    # Setup the source tar for rpm build
    git clone --recursive https://github.com/Seagate/cortx-s3server.git cortx-s3server-${S3_VERSION}-git${GIT_VER}
    cd cortx-s3server-${S3_VERSION}-git${GIT_VER}
    if [ $ENABLE_DEBUG_LOG == 1 ]; then
        sed -i 's/#logLevel=DEBUG.*$/logLevel=DEBUG/g' auth/resources/authserver.properties.sample
        sed -i 's/S3_LOG_MODE:.*$/S3_LOG_MODE: DEBUG/g' s3config.release.yaml.sample
    fi
    # For sake of test, attempt checkout of version
    git checkout ${GIT_VER}
elif ! [ -z "${PATH_SRC}" ]; then
    GIT_VER=$(git --git-dir "${PATH_SRC}"/.git rev-parse --short HEAD)
    mkdir -p cortx-s3server-${S3_VERSION}-git${GIT_VER}
    cp -ar "${PATH_SRC}"/. ./cortx-s3server-${S3_VERSION}-git${GIT_VER}
    if [ $ENABLE_DEBUG_LOG == 1 ]; then
        sed -i 's/#logLevel=DEBUG.*$/logLevel=DEBUG/g' cortx-s3server-${S3_VERSION}-git${GIT_VER}/auth/resources/authserver.properties.sample
        sed -i 's/S3_LOG_MODE:.*$/S3_LOG_MODE: DEBUG/g' cortx-s3server-${S3_VERSION}-git${GIT_VER}/s3config.release.yaml.sample
    fi
    find ./cortx-s3server-${S3_VERSION}-git${GIT_VER} -type f -name CMakeCache.txt -delete;
fi

cd ~/rpmbuild/SOURCES/
tar -zcvf cortx-s3server-${S3_VERSION}-git${GIT_VER}.tar.gz cortx-s3server-${S3_VERSION}-git${GIT_VER}
rm -rf cortx-s3server-${S3_VERSION}-git${GIT_VER}

cd ~/rpmbuild/SOURCES/

if [ -z "${DISABLE_MOTR}" ]; then
  yum-builddep -y ${BASEDIR}/s3rpm.spec

  rpmbuild -ba \
           --define "_s3_version ${S3_VERSION}" \
           --define "_s3_git_ver git${GIT_VER}" \
           ${BASEDIR}/s3rpm.spec --with python3

else
  yum-builddep -y ${BASEDIR}/s3rpm.spec --define "$DISABLE_MOTR"

  rpmbuild -ba \
           --define "_s3_version ${S3_VERSION}" \
           --define "_s3_git_ver git${GIT_VER}" \
           ${BASEDIR}/s3rpm.spec --with python3 \
           --define "$DISABLE_MOTR"
fi


if [ $INSTALL_AFTER_BUILD == 1 ]; then
    RPM_ARCH=$(rpm --eval "%{_arch}")
    RPM_DIST=$(rpm --eval "%{?dist:el7}")
    RPM_BUILD_VER=$(test -n "$build_number" && echo "$build_number" || echo 1 )
    RPM_PATH=~/rpmbuild/RPMS/${RPM_ARCH}/cortx-s3server-${S3_VERSION}-${RPM_BUILD_VER}_git${GIT_VER}_${RPM_DIST}.${RPM_ARCH}.rpm
    echo "Installing $RPM_PATH..."
    rpm -i $RPM_PATH
fi
