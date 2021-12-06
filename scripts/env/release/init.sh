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

install_toml() {
  echo "Installing toml"
  pip3 install toml
}

#function to install/upgrade cortx-py-utils rpm
install_cortx_py_utils() {
  #install yum-utils
  if rpm -q 'yum-utils' ; then
    echo "yum-utils already present ... Skipping ..."
  else
    yum install yum-utils -y
  fi

  #install cpio
  if rpm -q 'cpio' ; then
    echo "cpio already present ... Skipping ..."
  else
    yum install cpio -y
  fi

  # cleanup
  rm -rf "$PWD"/cortx-py-utils*
  rm -rf "$PWD"/opt

  # download cortx-py-utils.
  yumdownloader --destdir="$PWD" cortx-py-utils

  # extract requirements.txt
  rpm2cpio cortx-py-utils-*.rpm | cpio -idv "./opt/seagate/cortx/utils/conf/python_requirements.txt"
  rpm2cpio cortx-py-utils-*.rpm | cpio -idv "./opt/seagate/cortx/utils/conf/python_requirements.ext.txt"

  # install cortx-py-utils prerequisite
  pip3 install -r "$PWD/opt/seagate/cortx/utils/conf/python_requirements.txt"
  pip3 install -r "$PWD/opt/seagate/cortx/utils/conf/python_requirements.ext.txt"

  # install cortx-py-utils
  if rpm -q cortx-py-utils ; then
    yum remove cortx-py-utils -y
  fi
  yum install cortx-py-utils -y
}

# function to install all prerequisite for dev vm 
install_pre_requisites() {

  # install kafka server
  sh ${S3_SRC_DIR}/scripts/kafka/install-kafka.sh -c 1 -i $HOSTNAME

  #sleep for 60 secs to make sure all the services are up and running.
  sleep 60

  #create topic
  sh ${S3_SRC_DIR}/scripts/kafka/create-topic.sh -c 1 -i $HOSTNAME

  #install toml
  pip3 install toml

  # install or upgrade cortx-py-utils
  install_cortx_py_utils

  # install configobj
  pip3 install configobj
}

usage() {
  echo "Usage: $0
  optional arguments:
       -a    setup s3 release autonomously
       -h    show this help message and exit" 1>&2;
  exit 1; }

# validate and configure lnet
sh ${S3_SRC_DIR}/scripts/env/common/configure_lnet.sh

if [[ $# -eq 0 ]] ; then
  source ${S3_SRC_DIR}/scripts/env/common/setup-yum-repos.sh
  #install pre-requisites on dev vm
  install_pre_requisites
else
  while getopts "ah" x; do
      case "${x}" in
          a)
              yum install createrepo -y
              read -p "Git Access Token:" git_access_token
              source ${S3_SRC_DIR}/scripts/env/common/create-cortx-repo.sh -G $git_access_token
              # install configobj
              pip3 install configobj
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
# remove old openldap pkg if installed
yum remove -y openldap-servers openldap-clients || /bin/true
yum remove -y symas-openldap symas-openldap-servers symas-openldap-clients || /bin/true
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{2\}s3user.ldif
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
#yum install symas-openldap-servers haproxy -y # so we have "ldap" and "haproxy" users.
#yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-certs*
#yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-client-certs*

# Coping the ssl certificates
mkdir -p /etc/ssl

cp -R  ${BASEDIR}/../../../ansible/files/certs/* /etc/ssl/

# Setup using ansible
yum install -y ansible facter

cd ${BASEDIR}/../../../ansible

# Erase old haproxy rpm and later install latest haproxy version 1.8.14
rpm -q haproxy && rpm -e haproxy

# add /usr/local/bin to PATH
export PATH=$PATH:/usr/local/bin
echo $PATH


# Update ansible/hosts file with local ip
cp -f ./hosts ./hosts_local
sed -i "s/^xx.xx.xx.xx/127.0.0.1/" ./hosts_local

# Set up release node
ansible-playbook -i ./hosts_local --connection local setup_release_node.yml -v  -k  --extra-vars "s3_src=${S3_SRC_DIR}"

rm -f ./hosts_local

systemctl restart haproxy

cd ${CURRENT_DIR}
