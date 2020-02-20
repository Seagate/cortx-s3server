#!/bin/bash -e

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

s3server_config="/opt/seagate/s3/conf/s3config.yaml"
authserver_config="/opt/seagate/auth/resources/authserver.properties"
backgrounddelete_config="/opt/seagate/s3/s3backgrounddelete/config.yaml"
s3startsystem_script="/opt/seagate/s3/s3startsystem.sh"

s3_core_dir="/var/mero/s3server-*"
tmp_dir="/tmp/s3_support_bundle"
disk_usage="$tmp_dir/disk_usage.txt"
cpu_info="$tmp_dir/cpuinfo.txt"
ram_usage="$tmp_dir/ram_usage.txt"
s3server_pids="$tmp_dir/s3server_pids.txt"
haproxy_pids="$tmp_dir/haproxy_pids.txt"
m0d_pids="$tmp_dir/m0d_pids.txt"

if [[ -z "$bundle_id" || -z "$bundle_path" ]];
then
    echo "Invalid input parameters are received"
    echo "$USAGE"
    exit 1
fi

# 1. Get log directory path from config file
s3server_logdir=`cat $s3server_config | grep "S3_LOG_DIR:" | cut -f2 -d: | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`
authserver_logdir=`cat $authserver_config | grep "logFilePath=" | cut -f2 -d'=' | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`
backgrounddelete_logdir=`cat $backgrounddelete_config | grep "logger_directory:" | cut -f2 -d: | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`

# Check if auth serve log directory point to auth folder instead of "auth/server" in properties file
if [[ "$authserver_logdir" = *"auth/server" ]];
then
    authserver_logdir=${authserver_logdir/%"/auth/server"/"/auth"}
fi

## Add file/directory locations for bundling
# Collect s3 core files from mero location
if [ -d "$s3_core_dir" ];
then
    args=$args" "$s3_core_dir
fi

# Collect ldap logs if available
if [ -f "$ldap_log" ];
then
    args=$args" "$ldap_log
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

# Clean up temp files
cleanup_tmp_files(){
rm -rf /tmp/s3_support_bundle
}

## 2. Build tar.gz file with bundleid at bundle_path location
# Create folder with component name at given destination
mkdir -p $s3_bundle_location

# Build tar file
tar -czPf $s3_bundle_location/$bundle_name $args --warning=no-file-changed

# Check exit code of above operation
# While doing tar operation if file gets modified, 'tar' raises warning with
# exitcode 1 but it will tar that file.
# refer https://stackoverflow.com/questions/20318852/tar-file-changed-as-we-read-it
if [[ ("$?" != "0") && ("$?" != "1") ]];
then
    echo "Failed to generate S3 support bundle"
    cleanup_tmp_files
    exit 1
fi

# Clean up temp files
cleanup_tmp_files
echo "S3 support bundle generated successfully!!!"
