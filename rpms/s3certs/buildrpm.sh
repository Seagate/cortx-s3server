#!/bin/sh

# Note: This is currently interactive script since following script is
#  interactive: <s3 src>/scripts/ssl/generate_certificate.sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

TAG="s3dev"
# We use s3 server version for cert package versioning so to know certs package
# was created for which version of S3.
S3_VERSION=1.0.0

usage() { echo "Usage: $0 [-T <short tag/domain>] [-S <S3 version>] [-h Show help]" 1>&2; exit 1; }

while getopts ":T:S:h:" o; do
    case "${o}" in
        T)
            TAG=${OPTARG}
            ;;
        S)
            S3_VERSION=s3ver${OPTARG}
            ;;
        h)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

echo "Using [S3_VERSION=${S3_VERSION}] ..."
echo "Using [TAG=${TAG}] ..."

mkdir -p ~/rpmbuild/SOURCES/
cd ~/rpmbuild/SOURCES/
rm -rf stx-s3-certs*

# Setup the source tar for rpm build
S3_CERTS_SRC=stx-s3-certs-${S3_VERSION}-${TAG}

mkdir ${S3_CERTS_SRC}
cd ${S3_CERTS_SRC}

# Generate the certificates
${BASEDIR}/../../scripts/ssl/generate_certificate.sh
cp -r ./s3_certs_sandbox/* .
rm -rf ./s3_certs_sandbox

cd ~/rpmbuild/SOURCES/

tar -zcvf ${S3_CERTS_SRC}.tar.gz ${S3_CERTS_SRC}
rm -rf ${S3_CERTS_SRC}

rpmbuild -ba --define "_s3_certs_version ${S3_VERSION}"  \
         --define "_s3_certs_src ${S3_CERTS_SRC}"  \
         --define "_s3_domain_tag ${TAG}" ${BASEDIR}/s3certs.spec

rpmbuild -ba --define "_s3_certs_version ${S3_VERSION}"  \
         --define "_s3_certs_src ${S3_CERTS_SRC}"  \
         --define "_s3_domain_tag ${TAG}" ${BASEDIR}/s3clientcerts.spec
