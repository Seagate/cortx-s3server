#!/bin/sh

USAGE="USAGE: bash $(basename "$0") [--use_http] [--help | -h]

where:
--use_http         Use HTTP for ST's
--help (-h)        Display help"

use_http=0

if [ $# -eq 0 ]
then
  echo "Using HTTPS for system tests"
elif [ $# -gt 1 ]
then
  echo "Invald arguments passed";
        echo "$USAGE"
        exit 1
else
  case "$1" in
    --use_http ) use_http=1;
        echo "Using HTTP for system tests";
        ;;
    --help | -h )
        echo "$USAGE"
        exit 1
        ;;
    * )
        echo "Invald argument passed";
        echo "$USAGE"
        exit 1
        ;;
  esac
fi
set -xe

export PATH=/opt/seagate/s3/bin/:$PATH
echo $PATH

#git clone --recursive http://es-gerrit.xyus.xyratex.com:8080/s3server

S3_BUILD_DIR=`pwd`

USE_SUDO=
if [[ $EUID -ne 0 ]]; then
  command -v sudo || (echo "Script should be run as root or sudo required." && exit 1)
  USE_SUDO=sudo
fi

ulimit -c unlimited

# Few assertions - prerun checks
rpm -q haproxy
rpm -q stx-s3-certs
rpm -q stx-s3-client-certs
systemctl status haproxy

cd $S3_BUILD_DIR
$USE_SUDO systemctl is-active s3authserver 2>&1 > /dev/null
if [[ $? -eq 0 ]]; then
  $USE_SUDO systemctl stop s3authserver || echo "Cannot stop s3authserver services"
fi

# Check if mero build is cached and is latest, else rebuild mero as well
THIRD_PARTY=$S3_BUILD_DIR/third_party
MERO_SRC=$THIRD_PARTY/mero
BUILD_CACHE_DIR=$HOME/.seagate_src_cache

cd $MERO_SRC && current_mero_rev=`git rev-parse HEAD` && cd $S3_BUILD_DIR
cached_mero_rev=`cat $BUILD_CACHE_DIR/cached_mero.git.rev` || echo "Mero not cached at $BUILD_CACHE_DIR"

if [ "$current_mero_rev" != "$cached_mero_rev" ]
then
  # we need to rebuild mero, clean old cache
  rm -rf $BUILD_CACHE_DIR
fi

./rebuildall.sh --no-mero-rpm --use-build-cache

# Stop any old running S3 instances
echo "Stopping any old running s3 services"
$USE_SUDO ./dev-stops3.sh

$USE_SUDO systemctl stop s3authserver

# Stop any old running mero
cd $MERO_SRC
echo "Stopping any old running mero services"
$USE_SUDO ./m0t1fs/../clovis/st/utils/mero_services.sh stop || echo "Cannot stop mero services"
cd $S3_BUILD_DIR

# Clean up mero and S3 log and data dirs
$USE_SUDO rm -rf /mnt/store/mero/* /var/log/mero/* /var/mero/* \
                 /var/log/seagate/s3/* /var/log/seagate/auth/server/* \
                 /var/log/seagate/auth/tools/*

# Configuration setting for using HTTP connection
if [ $use_http -eq 1 ]
then
  $USE_SUDO sed -i 's/S3_ENABLE_AUTH_SSL:.*$/S3_ENABLE_AUTH_SSL: false/g' /opt/seagate/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/S3_AUTH_PORT:.*$/S3_AUTH_PORT: 9085/g' /opt/seagate/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/enableSSLToLdap=.*$/enableSSLToLdap=false/g' /opt/seagate/auth/resources/authserver.properties
  $USE_SUDO sed -i 's/enable_https=.*$/enable_https=false/g' /opt/seagate/auth/resources/authserver.properties
  $USE_SUDO sed -i 's/enableHttpsToS3=.*$/enableHttpsToS3=false/g' /opt/seagate/auth/resources/authserver.properties
fi

# Start mero for new tests
cd $MERO_SRC
echo "Starting new built mero services"
$USE_SUDO ./m0t1fs/../clovis/st/utils/mero_services.sh start
cd $S3_BUILD_DIR

# Ensure correct ldap credentials are present.
./scripts/enc_ldap_passwd_in_cfg.sh -l ldapadmin \
          -p /opt/seagate/auth/resources/authserver.properties

# Enable fault injection in AuthServer
$USE_SUDO sed -i 's/enableFaultInjection=.*$/enableFaultInjection=true/g' /opt/seagate/auth/resources/authserver.properties

$USE_SUDO systemctl restart s3authserver

# Start S3 gracefully, with max 3 attempts
echo "Starting new built s3 services"
retry=1
max_retries=3
statuss3=0
while [[ $retry -le $max_retries ]]
do
  statuss3=0
  $USE_SUDO ./dev-starts3.sh
  # Give it a second to start
  sleep 1
  ./iss3up.sh 1 || statuss3=$?

  if [ "$statuss3" == "0" ]; then
    echo "S3 service started successfully..."
    break
  else
    # Sometimes if mero is not ready, S3 may fail to connect
    # cleanup and restart
    $USE_SUDO ./dev-stops3.sh
    sleep 1
  fi
  retry=$((retry+1))
done

if [ "$statuss3" != "0" ]; then
  echo "Cannot start S3 service"
  tail -50 /var/log/seagate/s3/s3server.INFO
  exit 1
fi


# Add certificate to keystore
if [ $use_http -eq 0 ]
then
  $USE_SUDO keytool -delete -alias s3server -keystore /etc/pki/java/cacerts -storepass changeit >/dev/null || true
  $USE_SUDO keytool -import -trustcacerts -alias s3server -noprompt -file /etc/ssl/stx-s3-clients/s3/ca.crt -keystore /etc/pki/java/cacerts -storepass changeit
fi

# Run Unit tests and System tests
S3_TEST_RET_CODE=0
if [ $use_http -eq 1 ]
then
  ./runalltest.sh --no-mero-rpm --no-https || { echo "S3 Tests failed." && S3_TEST_RET_CODE=1; }
else
  ./runalltest.sh --no-mero-rpm || { echo "S3 Tests failed." && S3_TEST_RET_CODE=1; }
fi

# Disable fault injection in AuthServer
$USE_SUDO sed -i 's/enableFaultInjection=.*$/enableFaultInjection=false/g' /opt/seagate/auth/resources/authserver.properties

$USE_SUDO systemctl stop s3authserver || echo "Cannot stop s3authserver services"
$USE_SUDO ./dev-stops3.sh || echo "Cannot stop s3 services"

# Dump last log lines for easy lookup in jenkins
tail -50 /var/log/seagate/s3/s3server.INFO

# To debug if there are any errors
tail -50 /var/log/seagate/s3/s3server.ERROR || echo "No Errors"

cd $MERO_SRC
$USE_SUDO ./m0t1fs/../clovis/st/utils/mero_services.sh stop || echo "Cannot stop mero services"
cd $S3_BUILD_DIR

exit $S3_TEST_RET_CODE
