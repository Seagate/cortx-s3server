#!/bin/sh

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
S3_SRC_DIR="$BASEDIR/../../../"
CURRENT_DIR=`pwd`

source ${S3_SRC_DIR}/scripts/env/common/setup-yum-repos.sh

# Cleanup obsolete rpms if already installed on this system
yum remove -y log4cxx_eos log4cxx_eos-devel log4cxx_eos-debuginfo || /bin/true
yum remove -y eos-s3iamcli eos-s3iamcli-devel || /bin/true
yum remove -y eos-s3server eos-s3server-debuginfo || /bin/true
yum remove -y eos-core eos-core-devel eos-core-debuginfo || /bin/true

yum install -y ansible facter rpm-build

cd ${BASEDIR}/../../../ansible

# Update ansible/hosts file with local ip
cp -f ./hosts ./hosts_local
sed -i "s/^xx.xx.xx.xx/127.0.0.1/" ./hosts_local

# Setup rpmbuild env
ansible-playbook -i ./hosts_local --connection local rpm_build_env.yml -v  -k   --extra-vars "s3_src=${S3_SRC_DIR}"

rm -f ./hosts_local

cd ${CURRENT_DIR}
