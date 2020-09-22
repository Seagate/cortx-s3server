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
S3_SRC_DIR="$BASEDIR/../../../"
CURRENT_DIR=`pwd`

usage() {
  echo "Usage: $0
  optional arguments:
       -a    setup s3 release autonomously
       -h    show this help message and exit" 1>&2;
  exit 1; }

if [[ $# -eq 0 ]] ; then
  source ${S3_SRC_DIR}/scripts/env/common/setup-yum-repos.sh
else
  while getopts "ah" x; do
      case "${x}" in
          a)
              yum install createrepo -y
              easy_install pip
              read -p "Git Access Token:" git_access_token
              source ${S3_SRC_DIR}/scripts/env/common/create-cortx-repo.sh -G $git_access_token
              ;;
          *)
              usage
              ;;
      esac
  done
  shift $((OPTIND-1))
fi

# We are setting up new VM, so just attempt clean openldap
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

# Coping the ssl certificates
mkdir -p /etc/ssl

cp -R  ${BASEDIR}/../../../ansible/files/certs/* /etc/ssl/

# Generate random password for jks key and keystore passwords
sh ${S3_SRC_DIR}/scripts/create_auth_jks_password.sh

# Setup using ansible
yum install -y ansible facter

cd ${BASEDIR}/../../../ansible

# Update ansible/hosts file with local ip
cp -f ./hosts ./hosts_local
sed -i "s/^xx.xx.xx.xx/127.0.0.1/" ./hosts_local

# Set up release node
ansible-playbook -i ./hosts_local --connection local setup_release_node.yml -v  -k  --extra-vars "s3_src=${S3_SRC_DIR}"

rm -f ./hosts_local

systemctl restart haproxy

cd ${CURRENT_DIR}
