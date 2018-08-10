#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

GIT_VER=
S3_VERSION=1.0.0

usage() { echo "Usage: $0 -G <git version> [-S <S3 version>]" 1>&2; exit 1; }

while getopts ":G:S:" o; do
    case "${o}" in
        G)
            GIT_VER=${OPTARG}
            ;;
        S)
            S3_VERSION=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z "${GIT_VER}" ]; then
    usage
fi

echo "Using [S3_VERSION=${S3_VERSION}] ..."
echo "Using [GIT_VER=${GIT_VER}] ..."

cd ~/rpmbuild/SOURCES/
rm -rf s3server*

git clone http://gerrit.mero.colo.seagate.com:8080/s3server s3server-${S3_VERSION}-git${GIT_VER}
cd s3server-${S3_VERSION}-git${GIT_VER}
# For sake of test, attempt checkout of version
git checkout ${GIT_VER}
cd ~/rpmbuild/SOURCES/
tar -zcvf s3server-${S3_VERSION}-git${GIT_VER}.tar.gz s3server-${S3_VERSION}-git${GIT_VER}
rm -rf s3server-${S3_VERSION}-git${GIT_VER}

cd ~/rpmbuild/SOURCES/

rpmbuild -ba --define "_s3_version ${S3_VERSION}"  --define "_s3_git_ver git${GIT_VER}" ${BASEDIR}/s3rpm.spec
