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

# This is a temporary script which can be used to run s3_setup for single
# node again and again.

die_with_error () {
	echo "$1"
	exit -1
}

./clean_open_ldap_by_s3.sh || die_with_error "./clean_open_ldap_by_s3.sh failed"

yum install -y --nogpgcheck cortx-motr cortx-s3server openldap-servers openldap-clients &> /dev/null || die_with_error "pkg install failed!"

/opt/seagate/cortx/s3/bin/s3_setup None &> /dev/null || die_with_error "s3:post_install failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup None || die_with_error "s3:post_install failed!"
echo "s3:post_install passed!"

clustreid="353f5ad1-afa8-4a3f-b55f-cbae4a687107"
myhostname=$(hostname -f)
myip=$(/sbin/ip -o -4 addr list eth0 | awk '{print $4}' | cut -d/ -f1)
key=$(s3cipher --generate_key --const_key openldap)
iampasswd=$(s3cipher --encrypt --data 'ldapadmin' --key $key)
rootdnpasswd=$(s3cipher --encrypt --data 'seagate' --key $key)
s3_conf_file="/tmp/s3_confstore.json"

if [ -e $s3_conf_file ]; then
  mv $s3_conf_file $s3_conf_file.bak
fi

tee -a $s3_conf_file << END
{
 "cluster":
  {
   "cluster_id": "$clustreid",
   "node_count": "1",
   "server": [
    {
     "hostname": "$myhostname",
     "roles": "primary,openldap_master",
     "network":
      {
       "data":
        {
         "private_ip": "$myip"
        }
      }
    }],
   "openldap":
    {
     "sgiampassword": "$iampasswd",
     "rootdnpassword": "$rootdnpasswd"
    }
  }
}
END

/opt/seagate/cortx/s3/bin/s3_setup --updateclusterid --createauthjkspassword --configldap --confurl "json://$s3_conf_file" &> /dev/null || die_with_error "s3:config failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup --updateclusterid --createauthjkspassword --configldap --confurl "json://$s3_conf_file"  || die_with_error "s3:config failed!"
echo "s3:config passed!"

/opt/seagate/cortx/s3/bin/s3_setup --s3backgrounddelete --s3recoverytool --confurl "json://$s3_conf_file" &> /dev/null || die_with_error "s3:init failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup --s3backgrounddelete --s3recoverytool --confurl "json://$s3_conf_file" || die_with_error "s3:init failed!"
echo "s3:init passed w/o --setlogrotate!"

/opt/seagate/cortx/s3/bin/s3_setup --cleanup --confurl "json://$s3_conf_file" &> /dev/null || die_with_error "s3:cleanup failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup --cleanup --confurl "json://$s3_conf_file" || die_with_error "s3:cleanup failed!"
echo "s3:cleanup passed!"
