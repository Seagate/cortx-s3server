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

centos_release=`cat /etc/redhat-release | awk '/CentOS/ {print}'`
redhat_release=`cat /etc/redhat-release | awk '/Red Hat/ {print}'`

os_full_version=""
os_major_version=""
os_minor_version=""
os_build_num=""
ansible_automation=0


unsupported_os() {
  echo "S3 currently supports only CentOS 7.7.1908, CentOS 7.8.2003 or RHEL 7.7" 1>&2;
  exit 1;
}

check_supported_kernel() {
  kernel_version=$(uname -r)
  if [[ "$kernel_version" != 3.10.0-1062.* && "$kernel_version" != 3.10.0-1127.* ]]
  then
    echo "S3 supports kernel version: [3.10.0-1062.el7.x86_64] or [3.10.0-1127.el7.x86_64] only." 1>&2;
    exit 1
  fi
}

usage() {
  echo "Usage: $0
  optional arguments:
       -a    setup s3dev autonomously
       -h    show this help message and exit" 1>&2;
  exit 1; }

# OS and Kernel version checks
if [ ! -z "$centos_release" ]; then
  os_full_version=`cat /etc/redhat-release | awk  '{ print $4 }'`
  os_major_version=`echo $os_full_version | awk -F '.' '{ print $1 }'`
  os_minor_version=`echo $os_full_version | awk -F '.' '{ print $2 }'`
  os_build_num=`echo $os_full_version | awk -F '.' '{ print $3 }'`

  if [ "$os_major_version" = "7" ]; then
    if [[ "$os_minor_version" != "7" && "$os_minor_version" != "8" ]]; then
      unsupported_os
    elif [[ "$os_build_num" != "1908" && "$os_build_num" != "2003" ]]; then
      echo "CentOS build $os_build_num is currently not supported."
      exit 1
    else
      check_supported_kernel
    fi
  else
    unsupported_os
  fi
elif [ ! -z "$redhat_release" ]; then
  os_full_version=`cat /etc/redhat-release | awk  '{ print $7 }'`
  os_major_version=`echo $os_full_version | awk -F '.' '{ print $1 }'`
  os_minor_version=`echo $os_full_version | awk -F '.' '{ print $2 }'`

  if [[ "$os_major_version" = "7" && "$os_minor_version" = "7" ]]; then
    check_supported_kernel
  else
    unsupported_os
  fi
else
  unsupported_os
fi

if [[ $# -eq 0 ]] ; then
  source ${S3_SRC_DIR}/scripts/env/common/setup-yum-repos.sh
else
  while getopts "ahs" x; do
      case "${x}" in
          a)
              yum install createrepo -y
              easy_install pip
              read -p "Git Access Token:" git_access_token
              source ${S3_SRC_DIR}/scripts/env/common/create-cortx-repo.sh -G $git_access_token
              ;;
          s)
             source ${S3_SRC_DIR}/scripts/env/common/setup-yum-repos.sh
             ansible_automation=1;
             ;;
          *)
              usage
              ;;
      esac
  done
  shift $((OPTIND-1))
fi

yum install rpm-build -y

#It seems motr dependency script install s3cmd(2.0.0)
#for s3 system test we need patched s3cmd(1.6.1), which s3 ansible installs
rpm -q s3cmd && rpm -e s3cmd --nodeps

# if [ "$os_major_version" = "8" ]; then
#   yum install @development -y
# fi

# Erase gtest and gmock rpms, if any as most probably they will be of version 1.7.0
# We have 1.10.0 version gtest rpm (having both googletest and googlemock binaries)
# now in cortx-storage, which will be installed.
rpm -q gmock-devel && rpm -e gmock-devel
rpm -q gmock && rpm -e gmock

rpm -q gtest-devel && rpm -e gtest-devel
rpm -q gtest && rpm -e gtest

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

# Generate random password for jks key and keystore passwords
sh ${S3_SRC_DIR}/scripts/create_auth_jks_password.sh

# Configure dev env
yum install -y ansible facter


cd ${BASEDIR}/../../../ansible

#Install motr build dependencies

# TODO Currently motr is not supported for CentOS 8, when support is there remove below check
if [ "$os_major_version" = "7" ];
then
  ./s3motr-build-depencies.sh
fi

# configure backgrounddelete ST dependencies
./setup_backgrounddelete_config.sh

# Update ansible/hosts file with local ip
cp -f ./hosts ./hosts_local
sed -i "s/^xx.xx.xx.xx/127.0.0.1/" ./hosts_local

# Setup dev env
if [ $ansible_automation -eq 1 ]
then
   OPENLDAP_PASSWD=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 6 | head -n 1)
   LDAPADMIN_PASSWD=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 7 | head -n 1)
   ansible-playbook -i ./hosts_local --connection local setup_s3dev_centos77_8.yml -v --extra-vars "s3_src=${S3_SRC_DIR} openldappasswd=$OPENLDAP_PASSWD ldapiamadminpasswd=$LDAPADMIN_PASSWD"
else
   ansible-playbook -i ./hosts_local --connection local setup_s3dev_centos77_8.yml -v -k --extra-vars "s3_src=${S3_SRC_DIR}"
fi

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
