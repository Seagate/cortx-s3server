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
S3_SRC_DIR="$BASEDIR/../../../"
CURRENT_DIR=`pwd`

OS=$(cat /etc/os-release | grep -w ID | cut -d '=' -f 2)
VERSION=`cat /etc/os-release | sed -n 's/VERSION_ID=\"\([0-9].*\)\"/\1/p'`
major_version=`echo ${VERSION} | awk -F '.' '{ print $1 }'`

source ${S3_SRC_DIR}/scripts/env/common/setup-yum-repos.sh

yum install rpm-build -y

#It seems motr dependency script install s3cmd(2.0.0)
#for s3 system test we need patched s3cmd(1.6.1), which s3 ansible installs
rpm -q s3cmd && rpm -e s3cmd --nodeps

# Cleanup obsolete rpms if already installed on this system
yum remove -y log4cxx_eos log4cxx_eos-devel log4cxx_eos-debuginfo || /bin/true
yum remove -y eos-s3iamcli eos-s3iamcli-devel || /bin/true
yum remove -y eos-s3server eos-s3server-debuginfo || /bin/true
yum remove -y eos-core eos-core-devel eos-core-debuginfo || /bin/true

if [ "$os_major_version" = "8" ]; then
  yum install @development -y
fi

cd $BASEDIR

# Attempt ldap clean up since ansible openldap setup is not idempotent
systemctl stop slapd 2>/dev/null || /bin/true
yum remove -y openldap-servers openldap-clients || /bin/true
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif
rm -rf /var/lib/ldap/*
rm -f /etc/sysconfig/slapd* 2>/dev/null || /bin/true
rm -f /etc/openldap/slapd* 2>/dev/null || /bin/true
rm -rf /etc/openldap/slapd.d/*

# Tools for ssl certificate generation
yum install -y openssl java-1.8.0-openjdk-headless

# Generate the certificates rpms for dev setup
# clean up
#rm -f ~/rpmbuild/RPMS/x86_64/stx-s3-certs*
#rm -f ~/rpmbuild/RPMS/x86_64/stx-s3-client-certs*

#cd ${BASEDIR}/../../../rpms/s3certs
# Needs openssl and jre which are installed with rpm_build_env
#./buildrpm.sh -T s3dev

# install the built certs
#rpm -e stx-s3-certs stx-s3-client-certs || /bin/true
#yum install openldap-servers haproxy -y # so we have "ldap" and "haproxy" users.
#yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-certs*
#yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-client-certs*
# Coping the certificates

mkdir -p /etc/ssl

cp -R  ${BASEDIR}/../../../ansible/files/certs/* /etc/ssl/


# Configure dev env
yum install -y ansible facter


cd ${BASEDIR}/../../../ansible

#Install motr build dependencies

# TODO Currently motr is not supported for CentOS 8, when support is there remove below check
if [ "$major_version" = "7" ];
then
  ./s3motr-build-depencies.sh
fi

# Update ansible/hosts file with local ip
cp -f ./hosts ./hosts_local
sed -i "s/^xx.xx.xx.xx/127.0.0.1/" ./hosts_local

# Setup dev env
ansible-playbook -i ./hosts_local --connection local setup_s3dev_centos77_8.yml -v  -k --extra-vars "s3_src=${S3_SRC_DIR}"

rm -f ./hosts_local

systemctl restart haproxy

sed  -ie '/secure_path/s/$/:\/opt\/seagate\/cortx\/s3\/bin/' /etc/sudoers

if ! command -v python36 &>/dev/null; then
  if command -v python3.6 &>/dev/null; then
    ln -s "`command -v python3.6`" /usr/bin/python36
  else
    echo "Python v3.6 is not installed (neither python36 nor python3.6 are found in PATH)."
    exit 1
  fi
fi

cd ${CURRENT_DIR}
