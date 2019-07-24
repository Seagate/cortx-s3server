#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
CURRENT_DIR=`pwd`
hostnamectl set-hostname s3dev

OS=$(cat /etc/os-release | grep -w ID | cut -d '=' -f 2)

rpm -qa git | grep git -q
if [ $? != 0 ]; then
  yum install -y git
fi

if [ "$OS" = "\"rhel\"" ]; then
  # We need to checkout mero, so that we can run install-build-deps
  cd $BASEDIR/../../../
  if [ ! -d "./mero" ]; then
    git clone http://gerrit.mero.colo.seagate.com:8080/mero
  fi
  ./update-hosts.sh
  # Setup system for the build of mero
  echo "=========================================================================================================="
  echo ""
  echo "mero build setup script may fail due to unavailability of repository http://debuginfo.centos.org"
  echo "comment section 'configure 'Debuginfo' repository' in mero/scripts/provisioning/roles/base-os/tasks/main.yml"
  echo "and remove '/etc/yum.repos.d/debuginfo.repo' file"
  echo ""
  echo "For lnet, in file 'mero/scripts/provisioning/roles/lustre-client/tasks/main.yml'"
  echo "remove 'tcp(eth1)' and put appropriate interface instead of eth0 in "
  echo "'options lnet networks=tcp(eth1),tcp1(eth0) config_on_load=1'"
  echo "For example: 'options lnet networks=tcp1(ens33) config_on_load=1'"
  echo ""
  echo "Comment complete section 'install up-to-date tools from Software Collections repo' in"
  echo "mero/scripts/provisioning/roles/mero-build/tasks/main.yml"
  echo ""
  echo "Without above changes currently the setup won't succeed"
  echo "=========================================================================================================="
  read -p "Continue (y/n)?" choice
  case "$choice" in
    y|Y ) echo "continuing...";;
    n|N ) exit;;
    * ) exit;;
  esac
  subscription-manager repos --enable=*
  subscription-manager repos --disable=rhel-atomic-7-cdk-3.*
  subscription-manager repos --disable=rhel-7-server-extras-beta-*
  yum localinstall -y http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
  yum install -y perl-YAML-LibYAML
  #Got error -- Delta RPMs disabled because /usr/bin/applydeltarpm not installed.
  yum install -y deltarpm
  $BASEDIR/../../../mero/scripts/install-build-deps
  rpm -qa s3cmd | grep s3cmd -q
  if [ $? == 0 ]; then
    # S3 provisioning will take care of installing appropriate s3cmd version
    rpm -e s3cmd
  fi
  rm -rf mero
fi

rpm -qa python34 | grep python34 -q
if [ $? != 0 ]; then
  yum install -y python34
fi

python_version=`python3 --version`
if [[ $python_version != *"Python 3.4"* ]]; then
  cd /usr/bin/
  unlink /usr/bin/python3
  ln -s python3.4 /usr/bin/python3
  cd -
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
rm -f ~/rpmbuild/RPMS/x86_64/stx-s3-certs*
rm -f ~/rpmbuild/RPMS/x86_64/stx-s3-client-certs*

cd ${BASEDIR}/../../../rpms/s3certs
# Needs openssl and jre which are installed with rpm_build_env
./buildrpm.sh -T s3dev

# install the built certs
rpm -e stx-s3-certs stx-s3-client-certs || /bin/true
yum install openldap-servers haproxy -y # so we have "ldap" and "haproxy" users.
yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-certs*
yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-client-certs*

# Configure dev env
yum install -y ansible

cd ${BASEDIR}/../../../ansible

# Update ansible/hosts file with local ip
cp -f ./hosts ./hosts_local
sed -i "s/^xx.xx.xx.xx/127.0.0.1/" ./hosts_local

# Setup necessary repos
ansible-playbook -i ./hosts_local --connection local jenkins_yum_repos.yml -v  -k

# Setup dev env
ansible-playbook -i ./hosts_local --connection local setup_s3dev_centos75.yml -v  -k

rm -f ./hosts_local

systemctl restart haproxy

sed  -ie '/secure_path/s/$/:\/opt\/seagate\/s3\/bin/' /etc/sudoers

cd ${CURRENT_DIR}
