#!/bin/sh

die_with_error () {
	echo $1
	exit -1
}

/root/clean_open_ldap_by_s3.sh

if [ ! -e "/opt/seagate/cortx/s3/conf/s3_confstore.json" ]; then
	tee -a /opt/seagate/cortx/s3/conf/s3_confstore.json << END
{
  "cluster": {
    "cluster_id": "dummy-id",
    "cluster_hosts": "localhost"
  }
}
END
fi

yum install -y --nogpgcheck cortx-motr cortx-s3server openldap-servers openldap-clients &> /dev/null || die_with_error "pkg install failed!"

/opt/seagate/cortx/s3/bin/s3_setup None &> /dev/null || die_with_error "s3:post_install failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup None || die_with_error "s3:post_install failed!"
echo "s3:post_install passed!"

/opt/seagate/cortx/s3/bin/s3_setup --updateclusterid --createauthjkspassword --configldap &> /dev/null || die_with_error "s3:config failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup --updateclusterid --createauthjkspassword --configldap || die_with_error "s3:config failed!"
echo "s3:config passed!"

/opt/seagate/cortx/s3/bin/s3_setup --s3backgrounddelete --s3recoverytool &> /dev/null || die_with_error "s3:init failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup --s3backgrounddelete --s3recoverytool  || die_with_error "s3:init failed!"
echo "s3:init passed w/o --setlogrotate!"

/opt/seagate/cortx/s3/bin/s3_setup --cleanup &> /dev/null || die_with_error "s3:cleanup failed!"
# bash -x /opt/seagate/cortx/s3/bin/s3_setup --cleanup || die_with_error "s3:cleanup failed!"
echo "s3:cleanup passed!"
