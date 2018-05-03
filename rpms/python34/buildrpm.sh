#!/bin/sh

set -e

WHEEL_VERSION=0.24.0
JMESPATH_VERSION=0.9.0
XMLTODICT_VERSION=0.9.0
S3TRANSFER_VERSION=0.1.10
BOTOCORE_VERSION=1.6.0
BOTO3_VERSION=1.4.6

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

cd ~/rpmbuild/SOURCES/
rm -rf wheel*
rm -rf jmespath*
rm -rf xmltodict*
rm -rf s3transfer*
rm -rf botocore*
rm -rf boto3*


#wheel
wget https://pypi.python.org/packages/source/w/wheel/wheel-${WHEEL_VERSION}.tar.gz

#jmespath
wget https://pypi.python.org/packages/source/j/jmespath/jmespath-${JMESPATH_VERSION}.tar.gz

#xmltodict
wget https://pypi.python.org/packages/source/x/xmltodict/xmltodict-${XMLTODICT_VERSION}.tar.gz

#botocore
wget https://pypi.io/packages/source/b/botocore/botocore-${BOTOCORE_VERSION}.tar.gz

#s3transfer
wget https://pypi.io/packages/source/s/s3transfer/s3transfer-${S3TRANSFER_VERSION}.tar.gz

#boto3
wget https://pypi.io/packages/source/b/boto3/boto3-${BOTO3_VERSION}.tar.gz

#copy patches
cp ${BASEDIR}/botocore/botocore-1.5.3-fix_dateutil_version.patch ~/rpmbuild/SOURCES/

cd -

rpmbuild -ba ${BASEDIR}/wheel/python-wheel.spec --with python3
rpmbuild -ba ${BASEDIR}/jmespath/python-jmespath.spec --with python3
rpmbuild -ba ${BASEDIR}/xmltodict/python-xmltodict.spec --with python3
rpmbuild -ba ${BASEDIR}/botocore/python-botocore.spec --with python3
rpmbuild -ba ${BASEDIR}/s3transfer/python-s3transfer.spec --with python3
rpmbuild -ba ${BASEDIR}/boto3/python-boto3.spec --with python3

