#!/bin/sh

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

rm -f ./hosts_local

cd ${CURRENT_DIR}
