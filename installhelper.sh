#!/bin/sh
# Helper script for S3/Auth install.
#   - Creates necessary directories for installation, logging etc.
#   - Copies bin/libs/conf/service files etc.
#   - argument 1: directory prefix - where to create install dirs.
set -e

usage()
{
  echo "Invalid arguments."
  echo "Usage: ./installhelper.sh <DIR_PREFIX> [--release]"
  exit 1
}

if [ $# -eq 1 ] ; then
  IS_RELEASE=
elif [ $# -eq 2 ] ; then
  if [ "$2" != "--release" ] ; then
    usage
  else
    IS_RELEASE=1
  fi
else
  usage
fi

INSTALL_PREFIX=$1
AUTH_INSTALL_LOCATION=$INSTALL_PREFIX/opt/seagate/auth
S3_INSTALL_LOCATION=$INSTALL_PREFIX/opt/seagate/s3
S3_CONFIG_FILE_LOCATION=$INSTALL_PREFIX/opt/seagate/s3/conf
S3_LOG_ROTATE_FILE_LOCATION=$INSTALL_PREFIX/etc/cron.hourly
SERVICE_FILE_LOCATION=$INSTALL_PREFIX/lib/systemd/system
LOG_DIR_LOCATION=$INSTALL_PREFIX/var/log/seagate
NODEJS_DIR_LOCATION=$INSTALL_PREFIX/opt/seagate/s3/nodejs
RSYSLOG_CFG_DIR_LOCATION=$INSTALL_PREFIX/etc/rsyslog.d

rm -rf $AUTH_INSTALL_LOCATION
rm -rf $S3_INSTALL_LOCATION

mkdir -p $AUTH_INSTALL_LOCATION
mkdir -p $AUTH_INSTALL_LOCATION/resources
mkdir -p $AUTH_INSTALL_LOCATION/scripts
mkdir -p $S3_INSTALL_LOCATION/addb-plugin
mkdir -p $S3_INSTALL_LOCATION/bin
mkdir -p $S3_INSTALL_LOCATION/libevent
mkdir -p $S3_INSTALL_LOCATION/resources
mkdir -p $S3_INSTALL_LOCATION/scripts
mkdir -p $S3_CONFIG_FILE_LOCATION
mkdir -p $S3_LOG_ROTATE_FILE_LOCATION
mkdir -p $SERVICE_FILE_LOCATION
mkdir -p $LOG_DIR_LOCATION/s3
mkdir -p $LOG_DIR_LOCATION/auth
mkdir -p $LOG_DIR_LOCATION/auth/server
mkdir -p $LOG_DIR_LOCATION/auth/tools
mkdir -p $NODEJS_DIR_LOCATION
mkdir -p $RSYSLOG_CFG_DIR_LOCATION

# Copy the s3 dependencies
cp -R third_party/libevent/s3_dist/lib/* $S3_INSTALL_LOCATION/libevent/

# Copy the s3 server
cp bazel-bin/s3server $S3_INSTALL_LOCATION/bin/

# Copy cloviskvscli tool
cp bazel-bin/cloviskvscli $S3_INSTALL_LOCATION/bin/

# Copy addb plugin
cp bazel-bin/libs3addbplugin.so $S3_INSTALL_LOCATION/addb-plugin/

# Copy the resources
cp resources/s3_error_messages.json $S3_INSTALL_LOCATION/resources/s3_error_messages.json

# Copy the S3 Config option file
if [ -z "${IS_RELEASE}" ] ; then
  cp s3config.yaml $S3_CONFIG_FILE_LOCATION
else
  cp s3config.release.yaml ${S3_CONFIG_FILE_LOCATION}/s3config.yaml
fi

# Copy the S3 Audit Log Config file
cp s3server_audit_log.properties $S3_CONFIG_FILE_LOCATION

# Copy the S3 Clovis layout mapping file for different object sizes
cp s3_obj_layout_mapping.yaml $S3_CONFIG_FILE_LOCATION

# Copy the S3 Stats whitelist file
cp s3stats-whitelist.yaml $S3_CONFIG_FILE_LOCATION

# Copy the s3 server startup script for multiple instance
cp ./system/s3startsystem.sh $S3_INSTALL_LOCATION/

# Copy the s3 server daemon shutdown script for multiple instance via systemctl
cp ./system/s3stopsystem.sh $S3_INSTALL_LOCATION/

# Copy the s3 service file for systemctl multiple instance support.
cp ./system/s3server@.service $SERVICE_FILE_LOCATION

# Copy the s3 log rotate script for retaining recent modified log files
cp -f scripts/s3-logrotate/s3logfilerollover.sh $S3_LOG_ROTATE_FILE_LOCATION

# Copy the s3 background producer file for systemctl support.
cp s3backgrounddelete/s3backgroundproducer.service $SERVICE_FILE_LOCATION

# Copy the s3 background consumer file for systemctl support.
cp s3backgrounddelete/s3backgroundconsumer.service $SERVICE_FILE_LOCATION

# Copy Auth server jar to install location
cp -f auth/server/target/AuthServer-1.0-0.jar $AUTH_INSTALL_LOCATION/

# Copy Auth Password Encrypt Tool
cp -f auth/encryptcli/target/AuthPassEncryptCLI-1.0-0.jar $AUTH_INSTALL_LOCATION/

#Copy Auth Server resources to install location
cp -ru auth/resources/static $AUTH_INSTALL_LOCATION/resources/
cp -f auth/resources/authserver-log4j2.xml $AUTH_INSTALL_LOCATION/resources/
cp -f auth/resources/authencryptcli-log4j2.xml $AUTH_INSTALL_LOCATION/resources/
cp -f auth/resources/authserver.properties $AUTH_INSTALL_LOCATION/resources/
cp -f auth/resources/keystore.properties $AUTH_INSTALL_LOCATION/resources/
cp -f auth/resources/defaultAclTemplate.xml $AUTH_INSTALL_LOCATION/resources/
cp -f auth/resources/AmazonS3.xsd $AUTH_INSTALL_LOCATION/resources/

# Copy the auth server startup script.
cp startauth.sh $AUTH_INSTALL_LOCATION/

# Copy auth server Helper scripts
cp -f scripts/enc_ldap_passwd_in_cfg.sh $AUTH_INSTALL_LOCATION/scripts/

# Copy auth server Helper scripts
cp -f scripts/change_ldap_passwd.ldif $AUTH_INSTALL_LOCATION/scripts/

# Copy s3-sanity
cp -f s3-sanity-test.sh $S3_INSTALL_LOCATION/scripts/

# Copy the auth service file for systemctl support.
cp auth/server/s3authserver.service $SERVICE_FILE_LOCATION

# Copy nodejs binary
#cp third_party/nodejs/s3_dist/bin/node $NODEJS_DIR_LOCATION/

# Copy rsyslog config
cp ./scripts/rsyslog-tcp-audit.conf $RSYSLOG_CFG_DIR_LOCATION

exit 0
