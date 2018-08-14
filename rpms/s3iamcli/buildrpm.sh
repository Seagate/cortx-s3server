#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

GIT_VER=
S3IAMCLI_VERSION=1.0.0

usage() { echo "Usage: $0 -G <git short revision> [-S <S3 IAM CLI version>]" 1>&2; exit 1; }

while getopts ":G:S:" o; do
    case "${o}" in
        G)
            GIT_VER=${OPTARG}
            ;;
        S)
            S3IAMCLI_VERSION=${OPTARG}
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

echo "Using [S3IAMCLI_VERSION=${S3IAMCLI_VERSION}] ..."
echo "Using [GIT_VER=${GIT_VER}] ..."

cd ~/rpmbuild/SOURCES/
rm -rf s3server*
rm -rf s3iamcli*

# Setup the source tar for rpm build
git clone http://gerrit.mero.colo.seagate.com:8080/s3server s3server-${S3IAMCLI_VERSION}-git${GIT_VER}
cd s3server-${S3IAMCLI_VERSION}-git${GIT_VER}
# For sake of test, attempt checkout of version
git checkout ${GIT_VER}

cd auth-utils
mv s3iamcli s3iamcli-${S3IAMCLI_VERSION}-git${GIT_VER}
tar -zcvf s3iamcli-${S3IAMCLI_VERSION}-git${GIT_VER}.tar.gz s3iamcli-${S3IAMCLI_VERSION}-git${GIT_VER}

cp s3iamcli-${S3IAMCLI_VERSION}-git${GIT_VER}.tar.gz ~/rpmbuild/SOURCES/
cd ~/rpmbuild/SOURCES/
rm -rf s3server-${S3IAMCLI_VERSION}-git${GIT_VER}

rpmbuild -ba --define "_s3iamcli_version ${S3IAMCLI_VERSION}"  --define "_s3iamcli_git_ver git${GIT_VER}" ${BASEDIR}/s3iamcli.spec --with python3
