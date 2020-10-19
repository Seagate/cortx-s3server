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


USAGE="USAGE: bash $(basename "$0") [--use_http_client | --s3server_enable_ssl ]
                                    [--use_ipv6] [--skip_build] [--skip_tests]
                                    [--cleanup_only]
                                    [--fake_obj] [--fake_kvs | --redis_kvs] [--basic_test_only]
                                    [--local_redis_restart]
                                    [--callgraph /path/to/output/file]
                                    [--ldap_admin_pwd]
                                    [--generate_support_bundle]
                                    [--job_id]
                                    [--gid]
                                    [--help | -h]

where:
--use_http_client        Use HTTP client for ST's. Default is use HTTPS client.

--s3server_enable_ssl    Use ssl for s3server, by default its disabled

--use_ipv6               Use ipv6 for ST's

--skip_build             Do not run build step

--skip_tests             Do not run tests, exit before test run

--cleanup_only           Do cleanup and stop everything; don't run anything.

--fake_obj               Run s3server with stubs for motr object read/write ops

--fake_kvs               Run s3server with stubs for motr kvs put/get/delete
                         create idx/remove idx

--redis_kvs              Run s3server with redis stubs for motr kvs put/get/delete
                         create idx/remove idx

--basic_test_only        Do not run all the tests. Only basic s3cmd regression
                         tests will be run. If --fake* params provided, tests will use
                         zero filled objects

--local_redis_restart    In case redis server installed on local machine this option restarts redis-server

--callgraph              Generate valgrind call graph; Especially usefull
                         together with --basic_test_only option

                         Value: /path/to/output/file

--ldap_admin_pwd         LDAP admin password. If not specified, script will use password set in
                         file '/root/.s3_ldap_cred_cache.conf'

                         Value: string

--generate_support_bundle If specified, the 's3_bundle_generate.sh' script will be called at the end
                          of jenkins build, to save s3server logs at the given loction.

                          Value: String - absolute-path/where/logs/will/be/saved

--job_id                  The 'Cortx-PR-Build' pipeline jenkins job id. Specifiy this option
                          along with '--generate_support_bundle' option. The job_id value will
                          be used to create log directory name.

                          Value: Int

--gid                     The global id of the user, who has initiated the jenkins job.
                          Specifiy this option along with '--generate_support_bundle' option.
                          The job_id value will be used to create log directory name.

                          Value: Int

--help (-h)        Display help"

use_http_client=0
s3server_enable_ssl=0
use_ipv6=0
restart_haproxy=0
skip_build=0
cleanup_only=0
skip_tests=0
fake_obj=0
fake_kvs=0
redis_kvs=0
basic_test_only=0
callgraph_cmd=""
local_redis_restart=0
ldap_admin_pwd=
generate_support_bundle=
job_id=0
gid=0
ansible=0


if [ $# -eq 0 ]
then
  echo "Using HTTPS for system tests"
else
  while [ "$1" != "" ]; do
    case "$1" in
      --use_http_client ) use_http_client=1;
          if [ $s3server_enable_ssl -eq 1 ]
          then
            echo "Invalid argument --s3server_enable_ssl"
            echo "$USAGE"
            exit 1
          fi
          echo "Using HTTP for system tests";
          ;;
      --use_ipv6 ) use_ipv6=1;
          echo "Using ipv6 for system tests";
          ;;
      --s3server_enable_ssl ) s3server_enable_ssl=1;
          if [ $use_http_client -eq 1 ]
          then
            echo "Invalid argument --use_http_client"
            echo "$USAGE"
            exit 1
          fi
          echo "System tests with ssl enabled s3server";
          ;;
      --cleanup_only ) cleanup_only=1;
          skip_build=1;
          echo "Stop all services, cleanup data, and exit";
          ;;
      --skip_build ) skip_build=1;
          echo "Skip build step";
          ;;
      --skip_tests ) skip_tests=1;
          echo "Skip test step";
          ;;
      --fake_obj ) fake_obj=1;
          echo "Stubs for motr object read/write ops";
          ;;
      --fake_kvs ) fake_kvs=1;
          if [ $redis_kvs -eq 1 ]
          then
              echo "Redis kvs and fake kvs cannot be specified together"
              echo "$USAGE"
              exit 1
          fi
          echo "Stubs for motr kvs put/get/delete/create idx/remove idx";
          ;;
      --redis_kvs ) redis_kvs=1;
          if [ $fake_kvs -eq 1 ]
          then
              echo "Redis kvs and fake kvs cannot be specified together"
              echo "$USAGE"
              exit 1
          fi
          echo "Stubs for motr kvs put/get/delete/create idx/remove idx";
          ;;
      --basic_test_only ) basic_test_only=1;
          echo "Run basic s3cmd regression tests only";
          ;;
      --callgraph ) shift;
                    if [[ $1 =~ ^[[:space:]]*$ ]]
                    then
                        callgraph_cmd="--callgraph /tmp/callgraph.out";
                    else
                        callgraph_cmd="--callgraph $1";
                    fi
                    echo "Generate valgrind call graph with params $callgraph_cmd";
                    ;;
       --ldap_admin_pwd ) shift;
                          if [ ! -z "$1" ]; then
                              ldap_admin_pwd=$1;
                          fi
                          ;;
       --generate_support_bundle ) shift;
                                   if [ ! -z "$1" ]; then
                                      generate_support_bundle=$1;
                                   fi
                                   ;;
       --job_id ) shift;
                  if [ ! -z "$1" ]; then
                      job_id=$1;
                  fi
                  ;;
       --gid ) shift;
               if [ ! -z "$1" ]; then
                  gid=$1;
               fi
               ;;
      --local_redis_restart ) echo "redis-server will be restarted";
                              local_redis_restart=1;
                              ;;
      --automate_ansible ) ansible=1;
                           ;;
      --help | -h )
          echo "$USAGE"
          exit 1
          ;;
      * )
          echo "Invalid argument passed";
          echo "$USAGE"
          exit 1
          ;;
    esac
    shift
  done
fi
set -xe

S3_BUILD_DIR=`pwd`
ANSIBLE_DIR=$S3_BUILD_DIR/scripts/env/dev

USE_SUDO=
if [[ $EUID -ne 0 ]]; then
  command -v sudo || (echo "Script should be run as root or sudo required." && exit 1)
  USE_SUDO=sudo
fi

if [[ $ansible -eq 1 ]]; then
   sh $ANSIBLE_DIR/init.sh -s
fi

source /root/.s3_ldap_cred_cache.conf

if [ "$callgraph_cmd" != "" ]
then
    $USE_SUDO yum -q list installed valgrind || $USE_SUDO yum -y install valgrind || (echo "Valgrind package cannot be installed" && exit 1)
fi

export PATH=/opt/seagate/cortx/s3/bin/:$PATH
echo $PATH

#git clone --recursive https://github.com/Seagate/cortx-s3server.git


ulimit -c unlimited

# Few assertions - prerun checks
rpm -q haproxy
#rpm -q stx-s3-certs
#rpm -q stx-s3-client-certs
systemctl status haproxy

cd $S3_BUILD_DIR

if [ $cleanup_only -eq 1 ]; then
    set +e
    $USE_SUDO kill -9 `pgrep redis`
    set -e
fi

is_authsrv_running=1
$USE_SUDO systemctl is-active s3authserver 2>&1 > /dev/null || is_authsrv_running=0
if [[ $is_authsrv_running -eq 1 ]]; then
  $USE_SUDO systemctl stop s3authserver || echo "Cannot stop s3authserver services"
fi

# Check if motr build is cached and is latest, else rebuild motr as well
THIRD_PARTY=$S3_BUILD_DIR/third_party
MOTR_SRC=$THIRD_PARTY/motr
BUILD_CACHE_DIR=$HOME/.seagate_src_cache

cd $MOTR_SRC && current_motr_rev=`git rev-parse HEAD` && cd $S3_BUILD_DIR
cached_motr_rev=`cat $BUILD_CACHE_DIR/cached_motr.git.rev` || echo "Motr not cached at $BUILD_CACHE_DIR"

if [ "$current_motr_rev" != "$cached_motr_rev" ]
then
  # we need to rebuild motr, clean old cache
  rm -rf $BUILD_CACHE_DIR
fi

if [ $skip_build -eq 0 ]
then
    ./rebuildall.sh --no-motr-rpm --use-build-cache
fi

# Stop any old running S3 instances
echo "Stopping any old running s3 services"
$USE_SUDO ./dev-stops3.sh

$USE_SUDO systemctl stop s3authserver

# Stop any old running motr
cd $MOTR_SRC
echo "Stopping any old running motr services"
$USE_SUDO ./m0t1fs/../motr/st/utils/motr_services.sh stop || echo "Cannot stop motr services"
cd $S3_BUILD_DIR

# Clean up motr and S3 log and data dirs
$USE_SUDO rm -rf /mnt/store/motr/* /var/log/motr/* /var/motr/* \
                 /var/log/seagate/s3/* /var/log/seagate/auth/server/* \
                 /var/log/seagate/auth/tools/* /var/crash/*

if [ $cleanup_only -eq 1 ]; then
  exit
fi

# Configuration setting for using HTTP connection
if [ $use_http_client -eq 1 ]
then
  $USE_SUDO sed -i 's/S3_ENABLE_AUTH_SSL:.*$/S3_ENABLE_AUTH_SSL: false/g' /opt/seagate/cortx/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/S3_AUTH_PORT:.*$/S3_AUTH_PORT: 9085/g' /opt/seagate/cortx/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/enableSSLToLdap=.*$/enableSSLToLdap=false/g' /opt/seagate/cortx/auth/resources/authserver.properties
  $USE_SUDO sed -i 's/enable_https=.*$/enable_https=false/g' /opt/seagate/cortx/auth/resources/authserver.properties
  $USE_SUDO sed -i 's/enableHttpsToS3=.*$/enableHttpsToS3=false/g' /opt/seagate/cortx/auth/resources/authserver.properties
fi

# Configuration setting for ipv6 connection
if [ $use_ipv6 -eq 1 ]
then
  $USE_SUDO sed -i 's/S3_SERVER_IPV4_BIND_ADDR:.*$/S3_SERVER_IPV4_BIND_ADDR: ~/g' /opt/seagate/cortx/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/S3_SERVER_IPV6_BIND_ADDR:.*$/S3_SERVER_IPV6_BIND_ADDR: ::\/128/g' /opt/seagate/cortx/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/\(\s*S3_AUTH_IP_ADDR:\s*\)ipv4:[[:digit:]]*\.[[:digit:]]*\.[[:digit:]]*\.[[:digit:]]*\(\s*.*\)/\1ipv6:::1\2/g' /opt/seagate/cortx/s3/conf/s3config.yaml
  # backup
  $USE_SUDO \cp /etc/haproxy/haproxy.cfg{,.bak}
  $USE_SUDO sed -i 's/0\.0\.0\.0/::/g' /etc/haproxy/haproxy.cfg
  restart_haproxy=1
fi

if [ $s3server_enable_ssl -eq 1 ]
then
  $USE_SUDO sed -i 's/S3_SERVER_SSL_ENABLE:.*$/S3_SERVER_SSL_ENABLE: true/g' /opt/seagate/cortx/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/^\(\s*server\s\+s3-instance.*\)\ check/\1 check ssl verify required ca-file \/etc\/ssl\/stx-s3\/s3\/ca.crt/g' /etc/haproxy/haproxy.cfg
  restart_haproxy=1
fi

if [ $restart_haproxy -eq 1 ]
then
  $USE_SUDO systemctl restart haproxy
fi

# Copy jks and keystore.properties file from /root/.cortx_s3_auth_jks directory to Auth install directory
echo "Updating Authserver keystore with random password.."
JKS_DIR="/root/.cortx_s3_auth_jks"
if [ -d "$JKS_DIR" ]
then
    cp -f $JKS_DIR/s3authserver.jks /opt/seagate/cortx/auth/resources
    cp -f $JKS_DIR/keystore.properties /opt/seagate/cortx/auth/resources
else
    echo "Auth jksstore and keystore files are missing.Please re-run ansible (init.sh) script to regenerate these files."
    exit 1;
fi

# Start motr for new tests
cd $MOTR_SRC
echo "Starting new built motr services"
$USE_SUDO ./m0t1fs/../motr/st/utils/motr_services.sh start
cd $S3_BUILD_DIR

# Ensure correct ldap credentials are present.
./scripts/enc_ldap_passwd_in_cfg.sh -l "$ldap_admin_pwd" \
          -p /opt/seagate/cortx/auth/resources/authserver.properties

# Enable fault injection in AuthServer
$USE_SUDO sed -i 's/enableFaultInjection=.*$/enableFaultInjection=true/g' /opt/seagate/cortx/auth/resources/authserver.properties

$USE_SUDO systemctl restart s3authserver

# Start S3 gracefully, with max 3 attempts
echo "Starting new built s3 services"
retry=1
max_retries=3
statuss3=0
fake_params=""
if [ $fake_kvs -eq 1 ]
then
    fake_params+=" --fake_kvs"
fi

if [ $local_redis_restart -eq 1 ]; then
    rpm -q redis
    set +e
    $USE_SUDO kill -9 `pgrep redis`
    set -e
    redis-server --port 6379 &
fi

if [ $redis_kvs -eq 1 ]
then
    fake_params+=" --redis_kvs"
fi

if [ $fake_obj -eq 1 ]
then
    fake_params+=" --fake_obj"
fi

while [[ $retry -le $max_retries ]]
do
  statuss3=0
  $USE_SUDO ./dev-starts3.sh $fake_params $callgraph_cmd

  # Wait s3server to start
  timeout 2m bash -c "while ! ./iss3up.sh; do sleep 1; done"
  statuss3=$?

  if [ "$statuss3" == "0" ]; then
    echo "S3 service started successfully..."
    break
  else
    # Sometimes if motr is not ready, S3 may fail to connect
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
if [ $use_http_client -eq 0 ]
then
  $USE_SUDO keytool -delete -alias s3server -keystore /etc/pki/java/cacerts -storepass changeit >/dev/null || true
  $USE_SUDO keytool -import -trustcacerts -alias s3server -noprompt -file /etc/ssl/stx-s3-clients/s3/ca.crt -keystore /etc/pki/java/cacerts -storepass changeit
fi
use_ipv6_arg=""
if [ $use_ipv6 -eq 1 ]
then
  use_ipv6_arg="--use-ipv6"
fi

if [ $skip_tests -eq 1 ]
then
    echo "Skip tests. S3server will not be stopped"
    exit 1
fi

basic_test_cmd_par=""
if [ $basic_test_only -eq 1 ]
then
    basic_test_cmd_par="--basic-s3cmd-rand"
    if [ $fake_obj -eq 1 ]
    then
        basic_test_cmd_par="--basic-s3cmd-zero"
    fi
fi

# Run Unit tests and System tests
S3_TEST_RET_CODE=0
if [ $use_http_client -eq 1 ]
then
  ./runalltest.sh --no-motr-rpm --no-https $use_ipv6_arg $basic_test_cmd_par || { echo "S3 Tests failed." && S3_TEST_RET_CODE=1; }
else
  ./runalltest.sh --no-motr-rpm $use_ipv6_arg $basic_test_cmd_par || { echo "S3 Tests failed." && S3_TEST_RET_CODE=1; }
fi

# Disable fault injection in AuthServer
$USE_SUDO sed -i 's/enableFaultInjection=.*$/enableFaultInjection=false/g' /opt/seagate/cortx/auth/resources/authserver.properties

$USE_SUDO systemctl stop s3authserver || echo "Cannot stop s3authserver services"
$USE_SUDO ./dev-stops3.sh $callgraph_cmd || echo "Cannot stop s3 services"

if [ $s3server_enable_ssl -eq 1 ]
then
  # Restore the config file
  $USE_SUDO sed -i 's/S3_SERVER_SSL_ENABLE: true/S3_SERVER_SSL_ENABLE: false                          #Enable s3server SSL/g' /opt/seagate/cortx/s3/conf/s3config.yaml
  $USE_SUDO sed -i 's/^\(\s*server\s\+s3-instance.* check\).*$/\1/g' /etc/haproxy/haproxy.cfg
fi

# To debug if there are any errors
tail -50 /var/log/seagate/s3/s3server.ERROR || echo "No Errors"

# jenkins pipeline to give this argument.
if [ ! -z "$generate_support_bundle" ]
then
  /opt/seagate/cortx/s3/scripts/s3_bundle_generate.sh "$gid-$job_id" "$generate_support_bundle"
fi

cd $MOTR_SRC
$USE_SUDO ./m0t1fs/../motr/st/utils/motr_services.sh stop || echo "Cannot stop motr services"
cd $S3_BUILD_DIR
# revert ipv6 settings
if [ $use_ipv6 -eq 1 ]
then
  # revert ipv6 changes
  $USE_SUDO \mv /etc/haproxy/haproxy.cfg.bak /etc/haproxy/haproxy.cfg
  $USE_SUDO systemctl restart haproxy
fi

exit $S3_TEST_RET_CODE
