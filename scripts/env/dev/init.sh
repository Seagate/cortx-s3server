#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
CURRENT_DIR=`pwd`

yum install -y ansible

cd ${BASEDIR}/../../../ansible

# Update ansible/hosts file with local ip
cp -f ./hosts ./hosts_local
sed -i "s/^xx.xx.xx.xx/127.0.0.1/" ./hosts_local

# run ansible to configure build env
ansible-playbook -i ./hosts_local --connection local rpm_build_env.yml -v  -k

# Generate the certificates rpms for dev setup
# clean up
rm -f ~/rpmbuild/RPMS/x86_64/stx-s3-certs*
rm -f ~/rpmbuild/RPMS/x86_64/stx-s3-client-certs*

cd ${BASEDIR}/../../../rpms/s3certs
# Needs openssl and jre which are installed with rpm_build_env
./buildrpm.sh

# install the built certs
rpm -e stx-s3-certs stx-s3-client-certs || /bin/true
yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-certs*
yum localinstall -y ~/rpmbuild/RPMS/x86_64/stx-s3-client-certs*

# Configure dev env
# Attempt ldap clean up since ansible openldap setup is not idempotent
systemctl stop slapd || /bin/true
rpm -e openldap-servers openldap-clients || /bin/true
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif
rm -rf /var/lib/ldap/*
rm -f /etc/sysconfig/slapd*

cd ${BASEDIR}/../../../ansible
ansible-playbook -i ./hosts_local --connection local setup_s3dev_centos75.yml -v  -k

rm -f ./hosts_local

systemctl restart haproxy

cd ${CURRENT_DIR}
