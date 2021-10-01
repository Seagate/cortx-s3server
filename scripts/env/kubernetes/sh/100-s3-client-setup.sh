#!/bin/bash
#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

set -euo pipefail # exit on failures

source ./config.sh
source ./env.sh
source ./sh/functions.sh

set -x # print each statement before execution

add_separator CONFIGURING S3 CLIENTS.

yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/uploads/centos/centos-7.8.2003/s3server_uploads/
yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/github/main/centos-7.8.2003/last_successful/
yum install -y cortx-s3iamcli --nogpgcheck
pip3 install awscli
pip3 install awscli-plugin-endpoint

if ! which aws; then
  add_separator "AWS CLI installation failed"
  exit 1
fi

# Add Service IP address to /etc/hosts file, for s3.seagate.com and iam.seagate.com
sed -i \
  -e 's,\b\(dummy\.\)*\(iam\|s3\)\.seagate\.com\b,dummy.\2.seagate.com,g' \
  -e "/$CORTX_IO_SVC/d" \
  /etc/hosts

echo "$CORTX_IO_SVC s3.seagate.com iam.seagate.com" >> /etc/hosts

mkdir -p /var/log/seagate/auth/

s3iamcli ListAccounts --ldapuser sgiamadmin --ldappasswd ldapadmin --no-ssl

mkdir -p /root/.aws/
cat clients/aws.config > /root/.aws/config

mkdir -p /etc/ssl/stx-s3-clients/s3/
cat clients/ca.crt > /etc/ssl/stx-s3-clients/s3/ca.crt

s3iamcli CreateAccount -n admin -e admin@seagate.com \
    --ldapuser sgiamadmin --ldappasswd ldapadmin --no-ssl \
  > s3-account.txt

cat s3-account.txt

set_VAL_for_key() {
  key="$1"
  VAL=`cat s3-account.txt | sed 's/ *, */\n/g' | grep "$key" | sed "s,^$key *= *,,"`
}

set_VAL_for_key AccessKeyId
access_key="$VAL"

set_VAL_for_key SecretKey
secret_key="$VAL"

cat <<EOF > ~/.aws/credentials
[default]
aws_access_key_id = $access_key
aws_secret_access_key = $secret_key
EOF

add_separator SUCCESSFULLY CONFIGURED S3 CLIENTS.
