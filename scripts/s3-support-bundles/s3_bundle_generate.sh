#!/bin/bash -e
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


#######################################################
# Generate Support bundle for S3Server #
#######################################################

USAGE="USAGE: bash $(basename "$0") <bundleid> <path>
Generate support bundle for s3server.

where:
bundleid     Unique bundle-id used to identify support bundles.
path         Location at which support bundle needs to be copied."

set -e

if [ $# -lt 2 ]
then
  echo "$USAGE"
  exit 1
fi

bundle_id=$1
bundle_path=$2
bundle_name="s3_$bundle_id.tar.gz"
s3_bundle_location=$bundle_path/s3

haproxy_config="/etc/haproxy/haproxy.cfg"
haproxy_status_log="/var/log/haproxy-status.log"
haproxy_log="/var/log/haproxy.log"
ldap_log="/var/log/slapd.log"

s3server_config="/opt/seagate/cortx/s3/conf/s3config.yaml"
authserver_config="/opt/seagate/cortx/auth/resources/authserver.properties"
backgrounddelete_config="/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml"
s3startsystem_script="/opt/seagate/cortx/s3/s3startsystem.sh"
s3server_binary="/opt/seagate/cortx/s3/bin/s3server"
s3_motr_dir="/var/motr/s3server-*"
s3_core_dir="/var/crash"
sys_auditlog_dir="/var/log/audit"

# Create tmp folder with pid value to allow parallel execution
pid_value=$$
tmp_dir="/tmp/s3_support_bundle_$pid_value"
disk_usage="$tmp_dir/disk_usage.txt"
cpu_info="$tmp_dir/cpuinfo.txt"
ram_usage="$tmp_dir/ram_usage.txt"
s3server_pids="$tmp_dir/s3server_pids.txt"
haproxy_pids="$tmp_dir/haproxy_pids.txt"
m0d_pids="$tmp_dir/m0d_pids.txt"
s3_core_files="$tmp_dir/s3_core_files"
s3_m0trace_files="$tmp_dir/s3_m0trace_files"
first_s3_m0trace_file="$tmp_dir/first_s3_m0trace_file"
m0trace_files_count=5

# LDAP data
ldap_dir="$tmp_dir/ldap"
ldap_config="$ldap_dir/ldap_config_search.txt"
ldap_subschema="$ldap_dir/ldap_subschema_search.txt"
ldap_accounts="$ldap_dir/ldap_accounts_search.txt"
ldap_users="$ldap_dir/ldap_users_search.txt"

if [[ -z "$bundle_id" || -z "$bundle_path" ]];
then
    echo "Invalid input parameters are received"
    echo "$USAGE"
    exit 1
fi

# Check gzip is installed or not
# if its not installed, install it
if rpm -q "gzip"  > /dev/null;
then
    yum install -y gzip > /dev/null
fi

# 1. Get log directory path from config file
s3server_logdir=`cat $s3server_config | grep "S3_LOG_DIR:" | cut -f2 -d: | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`
authserver_logdir=`cat $authserver_config | grep "logFilePath=" | cut -f2 -d'=' | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`
backgrounddelete_logdir=`cat $backgrounddelete_config | grep "logger_directory:" | cut -f2 -d: | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`

# Compress each s3server core file present in /var/crash directory if available
# these compressed core file will be available in /tmp/s3_support_bundle_<pid>/s3_core_files directory
compress_core_files(){
  core_filename_pattern="*/core-s3server.*"
  for file in $s3_core_dir/*
  do
    if [[ -f "$file" && $file == $core_filename_pattern ]];
    then
        mkdir -p $s3_core_files
        file_name=$(basename "$file")      # e.g core-s3server.234678
        gzip -f -c $file > $s3_core_files/"$file_name".gz 2> /dev/null
    fi
  done
}

# Compress each m0trace files present in /var/motr/s3server-* directory if available
# compressed m0trace files will be available in /tmp/s3_support_bundle_<pid>/s3_m0trace_files/<s3instance-name>
compress_m0trace_files(){
  m0trace_filename_pattern="m0trace.*"
  dir="/var/motr"
  tmpr_dir="/var/m0trraces_tmp"
  cwd=$(pwd)
  cd $dir
  if [ -d "$tmpr_dir" ]; then
    rm -rf $tmpr_dir
  fi
  for d in s3server-*/;
  do
    cd $dir
    mkdir $tmpr_dir
    cd $d
    (ls -t $m0trace_filename_pattern | head -$m0trace_files_count) | xargs -I '{}' cp '{}' $tmpr_dir
    s3instance_name=$d   # e.g s3server-0x7200000000000000:0
    # compressed file path will be /tmp/s3_support_bundle_<pid>/s3_m0trace_files/<s3instance-name>
    compressed_file_path=$s3_m0trace_files/$s3instance_name
    mkdir -p $compressed_file_path
    cd $tmpr_dir
    for file in $tmpr_dir/*
    do
      if [[ -f "$file" ]];
      then
          file_name=$(basename "$file")
          gzip -f -c $file > $compressed_file_path/"$file_name".gz 2> /dev/null
      fi
    done
    rm -rf $tmpr_dir
  done
  cd $cwd
}

compress_first_m0trace_file(){
  dir="/var/motr"
  cwd=$(pwd)
  m0trace_filename_pattern="*/m0trace.*"
  cd $dir
  file_path=$(ls -t */m0trace* | tail -1)
  file="$(cut -d'/' -f2 <<<"$file_path")"
  compressed_file_path=$first_s3_m0trace_file
  mkdir -p $compressed_file_path
  gzip -f -c $file_path > $compressed_file_path/"$file".gz 2> /dev/null 
  cd $cwd
}

# Check if auth serve log directory point to auth folder instead of "auth/server" in properties file
if [[ "$authserver_logdir" = *"auth/server" ]];
then
    authserver_logdir=${authserver_logdir/%"/auth/server"/"/auth"}
fi

## Add file/directory locations for bundling

# Compress and collect s3 core files if available
compress_core_files
if [ -d "$s3_core_files" ];
then
    args=$args" "$s3_core_files
fi

compress_first_m0trace_file
if [ -d "$first_s3_m0trace_file" ];
then
   args=$args" "$first_s3_m0trace_file
fi

# Compress and collect m0trace files from /var/motr/s3server-* directory
# S3server name is generated with random name e.g s3server-0x7200000000000001:0x22
# check if s3server name with compgen globpat is available
if compgen -G $s3_motr_dir > /dev/null;
then
    compress_m0trace_files
    if [ -d "$s3_m0trace_files" ];
    then
        args=$args" "$s3_m0trace_files
    fi
fi

# Collect ldap logs if available
if [ -f "$ldap_log" ];
then
    args=$args" "$ldap_log
fi

# Collect System Audit logs if available
if [ -d "$sys_auditlog_dir" ];
then
    args=$args" "$sys_auditlog_dir
fi

# Collect s3 backgrounddelete logs if available
if [ -d "$backgrounddelete_logdir" ];
then
    args=$args" "$backgrounddelete_logdir
fi

# Collect s3server log directory if available
if [ -d "$s3server_logdir" ];
then
    args=$args" "$s3server_logdir
fi

# Collect authserver log directory if available
if [ -d "$authserver_logdir" ];
then
    args=$args" "$authserver_logdir
fi

# Collect s3 server config file if available
if [ -f "$s3server_config" ];
then
    args=$args" "$s3server_config
fi

# Collect authserver config file if available
if [ -f "$authserver_config" ];
then
    args=$args" "$authserver_config
fi

# Collect backgrounddelete config file if available
if [ -f "$backgrounddelete_config" ];
then
    args=$args" "$backgrounddelete_config
fi

# Collect s3startsystem script file if available
if [ -f "$s3startsystem_script" ];
then
    args=$args" "$s3startsystem_script
fi

# Collect s3server binary file if available
if [ -f "$s3server_binary" ];
then
    args=$args" "$s3server_binary
fi

# Collect haproxy config file if available
if [ -f "$haproxy_config" ];
then
    args=$args" "$haproxy_config
fi

# Collect haproxy log file if available
if [ -f "$haproxy_log" ];
then
    args=$args" "$haproxy_log
fi

# Collect haproxy status log file if available
if [ -f "$haproxy_status_log" ];
then
    args=$args" "$haproxy_status_log
fi

# Create temporary directory for creating other files as below
mkdir -p $tmp_dir

# Collect disk usage info
df -k > $disk_usage
args=$args" "$disk_usage

# Collect ram usage
free -h > $ram_usage
args=$args" "$ram_usage

# Colelct CPU info
cat /proc/cpuinfo > $cpu_info
args=$args" "$cpu_info

set +e

# Collect statistics of running s3server services
s3_pids=($(pgrep 's3server'))
if [ "${#s3_pids[@]}" -gt 0 ];
then
   top -b -n 1 "${s3_pids[@]/#/-p}" > $s3server_pids
   args=$args" "$s3server_pids
fi

# Collect statistics of running haproxy services
ha_pids=( $(pgrep 'haproxy') )
if [ "${#ha_pids[@]}" -gt 0 ];
then
   top -b -n 1 "${ha_pids[@]/#/-p}" > $haproxy_pids
   args=$args" "$haproxy_pids
fi

# Collect statistics of running m0d services
m0_pids=( $(pgrep 'm0d') )
if [ "${#m0_pids[@]}" -gt 0 ];
then
   top -b -n 1 "${m0_pids[@]/#/-p}" > $m0d_pids
   args=$args" "$m0d_pids
fi

## Collect LDAP data
mkdir -p $ldap_dir
# Fetch ldap root DN password from provisioning else use default
set +e
rootdnpasswd=""
if rpm -q "salt"  > /dev/null;
then
    rootdnpasswd=$(salt-call pillar.get openldap:admin:secret --output=newline_values_only)
    rootdnpasswd=$(salt-call lyveutil.decrypt openldap ${rootdnpasswd} --output=newline_values_only)
fi

if [[ -z "$rootdnpasswd" ]]
then
    rootdnpasswd="seagate"
fi

# Run ldap commands
ldapsearch -b "cn=config" -x -w $rootdnpasswd -D "cn=admin,cn=config" -H ldapi:/// > $ldap_config  2>&1
ldapsearch -s base -b "cn=subschema" objectclasses -x -w $rootdnpasswd -D "cn=admin,dc=seagate,dc=com" -H ldapi:/// > $ldap_subschema  2>&1
ldapsearch -b "ou=accounts,dc=s3,dc=seagate,dc=com" -x -w $rootdnpasswd -D "cn=admin,dc=seagate,dc=com" "objectClass=Account" -H ldapi:/// -LLL ldapentrycount > $ldap_accounts 2>&1
ldapsearch -b "ou=accounts,dc=s3,dc=seagate,dc=com" -x -w $rootdnpasswd -D "cn=admin,dc=seagate,dc=com" "objectClass=iamUser" -H ldapi:/// -LLL ldapentrycount > $ldap_users  2>&1

if [ -f "$ldap_config" ];
then
    args=$args" "$ldap_config
fi

if [ -f "$ldap_subschema" ];
then
    args=$args" "$ldap_subschema
fi

if [ -f "$ldap_accounts" ];
then
    args=$args" "$ldap_accounts
fi

if [ -f "$ldap_users" ];
then
    args=$args" "$ldap_users
fi

# Clean up temp files
cleanup_tmp_files(){
rm -rf /tmp/s3_support_bundle_$pid_value
}

## 2. Build tar.gz file with bundleid at bundle_path location
# Create folder with component name at given destination

mkdir -p $s3_bundle_location

# Build tar file
tar -czPf $s3_bundle_location/$bundle_name $args --warning=no-file-changed

# Check exit code of above operation
# While doing tar operation if file gets modified, 'tar' raises warning with
# exitcode 1 but it will tar that file.
if [[ ("$?" != "0") && ("$?" != "1") ]];
then
    echo "Failed to generate S3 support bundle"
    cleanup_tmp_files
    exit 1
fi

# Clean up temp files
cleanup_tmp_files
echo "S3 support bundle generated successfully!!!"
