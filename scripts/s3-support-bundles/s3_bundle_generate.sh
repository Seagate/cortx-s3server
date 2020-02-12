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

s3server_config="/opt/seagate/s3/conf/s3config.yaml"
authserver_config="/opt/seagate/auth/resources/authserver.properties"
backgrounddelete_config="/opt/seagate/s3/s3backgrounddelete/config.yaml"
s3startsystem_script="/opt/seagate/s3/s3startsystem.sh"

s3_core_dir="/var/mero/s3server-*"
disk_usage="/tmp/disk_usage.txt"
cpu_info="/tmp/cpuinfo.txt"
ram_usage="/tmp/ram_usage.txt"
service_pids="/tmp/service_pids.txt"

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
#TODO : gather LDAP logs

# Check if auth serve log directory point to auth folder instead of "auth/server" in properties file
if [[ "$authserver_logdir" = *"auth/server" ]];
then
    authserver_logdir=${authserver_logdir/%"/auth/server"/"/auth"}
fi

# Add files locations for bundling
args=$args" "$s3server_logdir
args=$args" "$authserver_logdir
args=$args" "$s3server_config
args=$args" "$authserver_config
args=$args" "$backgrounddelete_config
args=$args" "$s3startsystem_script
args=$args" "$haproxy_config
args=$args" "$haproxy_status_log
args=$args" "$haproxy_log

# Collect s3 core files from mero location
if [ -d $s3_core_dir ];
then
    args=$args" "$s3_core_dir
fi

# Collect s3 backgrounddelete logs
if [ -d $backgrounddelete_logdir ];
then
    args=$args" "$backgrounddelete_logdir
fi

# Collect disk usage info
df -k > $disk_usage
args=$args" "$disk_usage

# Collect ram usage
free -h > $ram_usage
args=$args" "$ram_usage

# Colelct CPU info
cat /proc/cpuinfo > $cpu_info
args=$args" "$cpu_info

# Collect pids of running services
pids=( $(pgrep 'haproxy|s3server|m0d') )
top -b -n 1 "${pids[@]/#/-p}" > $service_pids
args=$args" "$service_pids

## 2. Build tar.gz file with bundleid at bundle_path location
# Create folder with component name at given destination
mkdir -p $s3_bundle_location

# Build tar file
tar -czPf $s3_bundle_location/$bundle_name $args

# Clean up temp files
rm -rf $disk_usage
rm -rf $ram_usage
rm -rf $cpu_info
rm -rf $service_pids

echo "Bundled s3 server logs successfuly!!"
