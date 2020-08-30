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


unsupported_os() {
  echo "S3 currently supports only CentOS 7.7.1908 or RHEL 7.7" 1>&2;
  # exit 1; 
}

check_supported_kernel() {
  kernel_version=`uname -r`
  if [[ "$kernel_version" != 3.10.0-1062.* ]]; then
        echo "S3 supports kernel 3.10.0-1062.el7.x86_64" 1>&2;
        # exit 1
  fi }

usage() {
  echo "Usage: $0
  optional arguments:
       -a    setup s3dev autonomously
       -h    show this help message and exit" 1>&2;
  exit 1; }

if [ ! -z "$centos_release" ]; then
  os_full_version=`cat /etc/redhat-release | awk  '{ print $4 }'`
  os_major_version=`echo $os_full_version | awk -F '.' '{ print $1 }'`
  os_minor_version=`echo $os_full_version | awk -F '.' '{ print $2 }'`
  os_build_num=`echo $os_full_version | awk -F '.' '{ print $3 }'`
elif [ ! -z "$redhat_release" ]; then
  os_full_version=`cat /etc/redhat-release | awk  '{ print $7 }'`
  os_major_version=`echo $os_full_version | awk -F '.' '{ print $1 }'`
  os_minor_version=`echo $os_full_version | awk -F '.' '{ print $2 }'`
fi

# OS version and Kernel Checks
if [ "$os_major_version" = "7" ]; then
  if [ "$os_minor_version" = "7" ]; then
    # Centos 7.7
    if [ ! -z "$centos_release" ]; then
      if [ "$os_build_num" != "1908" ]; then
        echo "CentOS build $os_build_num is currently not supported"
        exit 1
      fi
      check_supported_kernel
    # RHEL 7.7
    elif [ ! -z "$redhat_release" ]; then
      check_supported_kernel
    # Other OS 7.7
    else
      unsupported_os
    fi
  else
    unsupported_os
  fi
else
 unsupported_os
fi

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

yum install rpm-build -y

#It seems motr dependency script install s3cmd(2.0.0)
#for s3 system test we need patched s3cmd(1.6.1), which s3 ansible installs
rpm -q s3cmd && rpm -e s3cmd --nodeps

# if [ "$os_major_version" = "8" ]; then
#   yum install @development -y
# fi

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
if [ "$os_major_version" = "7" ];
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
