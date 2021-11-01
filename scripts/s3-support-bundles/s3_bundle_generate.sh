#!/bin/bash
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

USAGE="USAGE: bash $(basename "$0") -b <bundleid> -t <path> -c <confstore_url> -s <services>
Generate support bundle for s3server.

where:
-b        Unique bundle-id used to identify support bundles.
-t        Location at which support bundle needs to be copied.
-c        Confstore URL.
-s        services list."

usage() {
  echo "$USAGE"
  exit 1
}

while getopts "b:t:c:s:" opt; do
  case "${opt}" in
    b) bundle_id=${OPTARG} ;;
    t) bundle_path=${OPTARG} ;;
    c) confstore_url=${OPTARG} ;;
    s) services=${OPTARG} ;;
    *) usage ;;
  esac
done

if [ -z "$bundle_id" ] || [ -z "$bundle_path" ] || [ -z "$confstore_url" ] || [ -z "$services" ] ; then
  usage
fi

echo "Bundle_id: $bundle_id"
echo "bundle_path: $bundle_path"
echo "confstore_url: $confstore_url"
echo "services: $services"


s3server_base_log_key=$(s3confstore "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml" getkey --key="CONFIG>CONFSTORE_BASE_LOG_PATH")
s3server_base_config_key=$(s3confstore "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml" getkey --key="CONFIG>CONFSTORE_BASE_CONFIG_PATH")

base_config_file_path=$(s3confstore "yaml://$confstore_url" getkey --key="$s3server_base_config_key")
base_log_file_path=$(s3confstore "yaml://$confstore_url" getkey --key="$s3server_base_log_key")
echo "base_config_file_path: $base_config_file_path"
echo "base_log_file_path: $base_log_file_path"

# Fetch iamuser password from properties file and decrypt it.
sgiamadminpwd=''
constkey=cortx
propertiesfilepath="properties://$base_config_file_path/auth/resources/authserver.properties"
ldapkey="ldapLoginPW"
ldapcipherkey=$(s3cipher generate_key --const_key="$constkey")
encryptedkey=$(s3confstore "$propertiesfilepath" getkey --key="$ldapkey")
if [[ -z $(echo "$encryptedkey" | grep -Eio Failed) ]];
then
    sgiamadminpwd=$(s3cipher decrypt --data="$encryptedkey" --key="$ldapcipherkey")
fi

bundle_name="s3_$bundle_id.tar.xz"
s3_bundle_location=$bundle_path/s3

haproxy_config="/etc/haproxy/haproxy.cfg"
# Collecting rotated logs for haproxy and ldap along with live log
haproxy_log="$base_log_file_path/haproxy.log"
haproxy_status_log="$base_log_file_path/haproxy-status.log"
haproxy_log_k8s=$(s3confstore "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml" getkey --key="S3_HAPROXY_LOG_SYMLINK")

s3server_config="$base_config_file_path/s3/conf/s3config.yaml"
authserver_config="$base_config_file_path/auth/resources/authserver.properties"
backgrounddelete_config="$base_config_file_path/s3/s3backgrounddelete/config.yaml"
s3cluster_config="$base_config_file_path/s3/s3backgrounddelete/s3_cluster.yaml"
s3startsystem_script="/opt/seagate/cortx/s3/s3startsystem.sh"
s3server_binary="/opt/seagate/cortx/s3/bin/s3server"

sys_auditlog_dir="/var/log/audit"

# S3 deployment log

s3deployment_log="$base_log_file_path/s3/s3deployment/s3deployment.log"


# Create tmp folder with pid value to allow parallel execution
pid_value=$$
tmp_dir="$s3_bundle_location/s3_support_bundle_$pid_value"
disk_usage="$tmp_dir/disk_usage.txt"
cpu_info="$tmp_dir/cpuinfo.txt"
ram_usage="$tmp_dir/ram_usage.txt"
s3server_pids="$tmp_dir/s3server_pids.txt"
haproxy_pids="$tmp_dir/haproxy_pids.txt"
m0d_pids="$tmp_dir/m0d_pids.txt"
node_port_info="$tmp_dir/node_port_info.txt"
rpm_info="$tmp_dir/s3_rpm_info.txt"
s3_core_files="$tmp_dir/s3_core_files"
s3_m0trace_files="$tmp_dir/s3_m0trace_files"
first_s3_m0trace_file="$tmp_dir/first_s3_m0trace_file"
m0trace_files_count=5
s3_core_files_max_count=11
max_allowed_core_size=10737418240 #e.g 10GB byte value
compress_core_file=true

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
s3server_logdir=$(s3confstore "yaml://$s3server_config" getkey --key="S3_SERVER_CONFIG>S3_LOG_DIR")
authserver_logdir=$(s3confstore "properties://$authserver_config" getkey --key="logFilePath")
backgrounddelete_producer_logdir=$(s3confstore "yaml://$backgrounddelete_config" getkey --key="logconfig>scheduler_logger_directory")
backgrounddelete_consumer_logdir=$(s3confstore "yaml://$backgrounddelete_config" getkey --key="logconfig>processor_logger_directory")
s3_motr_dir=$(s3confstore "yaml://$s3server_config" getkey --key="S3_SERVER_CONFIG>S3_DAEMON_WORKING_DIR")
s3_core_dir=$(s3confstore "yaml://$s3server_config" getkey --key="S3_SERVER_CONFIG>S3_DAEMON_WORKING_DIR")

# Collect call stack of latest <s3_core_files_max_count> s3server core files
# from s3_core_dir directory, if available
collect_core_files(){
  echo "Collecting core files..."
  # core_filename_pattern="core-s3server.*.gz"
  core_filename_pattern="core.*"
  mkdir -p $s3_core_files
  cwd=$(pwd)
  if [ ! -d "$s3_core_dir" ];
  then
      return;
  fi

  cd $s3_core_dir
  # get recent modified core files from directory
  (find . -name $core_filename_pattern 2>/dev/null | head -$s3_core_files_max_count) | xargs -I '{}' cp '{}' $s3_core_files

  # check for empty directory for core files
  if [ -z "$(ls -A $s3_core_files)" ];
  then
      rm -rf $s3_core_files
  else
      # iterate over the s3_core_files directory and extract call stack for each file
      for s3corefile in "$s3_core_files"/*
      do
         #zipped_core_name=$(basename "$s3corefile")
         #core_name=${zipped_core_name%".gz"}
         core_name=$(basename "$s3corefile")
         callstack_file="$s3_core_files"/callstack."$core_name".txt
         # Check the uncompressed size of gz core file
         #core_size=$(gunzip -lq $s3corefile)
         #uncompressed_size=$(cut -d' ' -f2 <<< $core_size)

         uncompressed_size=$(du -k "$s3corefile" | cut -f1)
         if [ $uncompressed_size -gt $max_allowed_core_size ];
         then
             echo "Ignoring $core_name because of core size($uncompressed_size Bytes) is greater than 10GB(10737418240 Bytes)" >> ./ignored_core_files.txt
             rm -f "$s3corefile"
         else
             printf "Core name: $core_name\n" >> "$callstack_file"
             printf "Callstack:\n\n" >> "$callstack_file"
             # compress and collect core file
             if [ $compress_core_file == true ];
             then
                 echo "Compress and collect s3 core file: $core_name"
                 gzip --fast $s3corefile 2>/dev/null
             else
                 gunzip $s3corefile 2>/dev/null
                 # unzip the core file, to read it using gdb
                 # generate gdb bt and append into the callstack_file
                 gdb --batch --quiet -ex "thread apply all bt full" -ex "quit" $s3server_binary\
                 "$s3_core_files/$core_name" 2>/dev/null >> "$callstack_file"
                 printf "\n**************************************************************************\n" >> "$callstack_file"

                 # generate gdb registers dump
                 printf "Register state:\n\n" >> "$callstack_file"
                 gdb --batch --quiet -ex "info registers" -ex "quit" $s3server_binary\
                 "$s3_core_files/$core_name" 2>/dev/null >> "$callstack_file"
                 printf "\n**************************************************************************\n" >> "$callstack_file"
                 # generate gdb assembly code
                 printf "Assembly code:\n\n" >> "$callstack_file"
                 gdb --batch --quiet -ex "disassemble" -ex "quit" $s3server_binary\
                 "$s3_core_files/$core_name" 2>/dev/null >> "$callstack_file"
                 printf "\n**************************************************************************\n" >> "$callstack_file"

                 # generate gdb memory map
                 printf "Memory map:\n\n" >> "$callstack_file"
                 gdb --batch --quiet -ex "info proc mapping" -ex "quit" $s3server_binary\
                 "$s3_core_files/$core_name" 2>/dev/null >> "$callstack_file"
                 printf "\n**************************************************************************\n" >> "$callstack_file"

                 # delete the inflated core file
                 rm -f "$s3_core_files/$core_name"
             fi
             # delete the copied core file
             rm -f "$s3corefile"
         fi
      done
  fi
  cd $cwd
}

# Collect <m0trace_files_count> m0trace files from each s3 instance present in /var/log/cortx/motr/s3server-* directory if available
# Files will be available at $tmp_path/s3_support_bundle_<pid>/s3_m0trace_files/<s3instance-name>
collect_m0trace_files(){
  echo "Collecting m0trace files dump..."
  m0trace_filename_pattern="m0trace.*"

  dir="$base_log_file_path/motr"
  tmpr_dir="$tmp_dir/m0trraces_tmp"
  cwd=$(pwd)
  # if $base_log_file_path/motr missing then return

  if [ ! -d "$dir" ];
  then
      return;
  fi

  cd $s3_motr_dir
  for s3_dir in s3server-*/;
  do
    cd $s3_motr_dir
    mkdir -p $tmpr_dir
    cd $s3_dir
    (ls -t $m0trace_filename_pattern 2>/dev/null | head -$m0trace_files_count) | xargs -I '{}' cp '{}' $tmpr_dir
    # No m0trace
    if [ -z "$(ls -A $tmpr_dir)" ];
    then
        rm -rf $tmpr_dir
        return;
    fi
    s3instance_name=$s3_dir   # e.g s3server-0x7200000000000000:0

    # m0trace file path will be /var/log/cortx/s3_support_bundle_<pid>/s3_m0trace_files/<s3instance-name>
    m0trace_file_path=$s3_m0trace_files/$s3instance_name
    mkdir -p $m0trace_file_path
    cd $tmpr_dir
    for file in $tmpr_dir/*
    do
      if [[ -f "$file" ]];
      then
          file_name=$(basename "$file")
          m0tracedump -s -i $file 2>/dev/null | xz --thread=0 > $m0trace_file_path/"$file_name".yaml.xz 2>/dev/null
      fi
    done
    rm -rf $tmpr_dir
  done
  cd $cwd

  # check for empty directory for m0trace files
  if [ -z "$(ls -A $m0trace_file_path)" ];
  then
      rm -rf $m0trace_file_path
  fi
}

collect_first_m0trace_file(){
  echo "Collecting oldest m0trace file dump..."
  dir="$base_log_file_path/motr"
  cwd=$(pwd)
  m0trace_filename_pattern="*/m0trace.*"
  if [ ! -d "$s3_motr_dir" ];
  then
      return;
  fi

  cd $s3_motr_dir
  file_path=$(ls -t */m0trace* 2>/dev/null | tail -1)
  # if m0trace are available
  if [ -f "$file_path" ];
  then
      file="$(cut -d'/' -f2 <<<"$file_path")"
      m0trace_file_path=$first_s3_m0trace_file
      mkdir -p $m0trace_file_path
      m0tracedump -s -i $file_path 2>/dev/null | xz -k --thread=0 > $m0trace_file_path/"$file".yaml.xz 2>/dev/null
  fi
  cd $cwd

  # check for empty directory for m0trace files
  if [ -z "$(ls -A $m0trace_file_path)" ];
  then
      rm -rf $m0trace_file_path
  fi
}

# Check if auth serve log directory point to auth folder instead of "auth/server" in properties file
if [[ "$authserver_logdir" = *"auth/server" ]];
then
    authserver_logdir=${authserver_logdir/%"/auth/server"/"/auth"}
fi

## Add file/directory locations for bundling
args=()

# Collect S3 deployment log
if [ -f "$s3deployment_log" ];
then
    args+=($s3deployment_log*)
fi

# Collect s3 core files if available
collect_core_files
if [ -d "$s3_core_files" ];
then
    args+=($s3_core_files)
fi

collect_first_m0trace_file
if [ -d "$first_s3_m0trace_file" ];
then
    args+=($first_s3_m0trace_file)
fi

# collect latest 5 m0trace files from /var/log/cortx/motr/s3server-* directory
# S3server name is generated with random name e.g s3server-0x7200000000000001:0x22
# check if s3server name with compgen globpat is available
if compgen -G $s3_motr_dir > /dev/null;
then
    collect_m0trace_files
    if [ -d "$s3_m0trace_files" ];
    then
        args+=($s3_m0trace_files)
    fi
fi

# Collect System Audit logs if available
if [ -d "$sys_auditlog_dir" ];
then
    args+=($sys_auditlog_dir)
fi

# Collect s3 backgrounddelete producer logs if available
if [ -d "$backgrounddelete_producer_logdir" ];
then
    args+=($backgrounddelete_producer_logdir)
fi

# Collect s3 backgrounddelete consumer logs if available
if [ -d "$backgrounddelete_consumer_logdir" ];
then
    args+=($backgrounddelete_consumer_logdir)
fi

# Collect s3server log directory if available
if [ -d "$s3server_logdir" ];
then
    args+=($s3server_logdir)
fi

# Collect authserver log directory if available
if [ -d "$authserver_logdir" ];
then
    args+=($authserver_logdir)
fi

# Collect s3 server config file if available
if [ -f "$s3server_config" ];
then
    args+=($s3server_config)
fi

# Collect authserver config file if available
if [ -f "$authserver_config" ];
then
    args+=($authserver_config)
fi

# Collect backgrounddelete config file if available
if [ -f "$backgrounddelete_config" ];
then
    args+=($backgrounddelete_config)
fi

# Collect s3cluster config file if available
if [ -f "$s3cluster_config" ];
then
    args+=($s3cluster_config)
fi

if [ -f "$s3startsystem_script" ];
then
    args+=($s3startsystem_script)
fi

# Collect s3server binary file if available
if [ -f "$s3server_binary" ];
then
    args+=($s3server_binary)
fi

# Collect haproxy config file if available
if [ -f "$haproxy_config" ];
then
    args+=($haproxy_config)
fi

# Collect haproxy log along with rotated logs if available
if [ -f "$haproxy_log" ];
then
    args+=($haproxy_log*)
fi

# Collect haproxy status log along with rotated logs if available
if [ -f "$haproxy_status_log" ];
then
    args+=($haproxy_status_log*)
fi

# Collect haproxy k8s log along with rotated logs if available
if [ -f "$haproxy_log_k8s" ];
then
    args+=($haproxy_log_k8s*)
fi

# Create temporary directory for creating other files as below
mkdir -p $tmp_dir

# Collect disk usage info
df -k > $disk_usage
args+=($disk_usage)

# Collect ram usage
free -h > $ram_usage
args+=($ram_usage)

# Colelct CPU info
cat /proc/cpuinfo > $cpu_info
args+=($cpu_info)

# Collect listening port numbers
netstat -tulpn | grep -i listen > $node_port_info 2>&1
args+=($node_port_info)

# Collect rpm package names of s3
rpm -qa | grep cortx-s3 > $rpm_info 2>&1
args+=($rpm_info)

# Collect statistics of running s3server services
s3_pids=($(pgrep 's3server'))
if [ "${#s3_pids[@]}" -gt 0 ];
then
   top -b -n 1 "${s3_pids[@]/#/-p}" > $s3server_pids
   args+=($s3server_pids)
fi

# Collect statistics of running haproxy services
ha_pids=( $(pgrep 'haproxy') )
if [ "${#ha_pids[@]}" -gt 0 ];
then
   top -b -n 1 "${ha_pids[@]/#/-p}" > $haproxy_pids
   args+=($haproxy_pids)
fi

# Collect statistics of running m0d services
m0_pids=( $(pgrep 'm0d') )
if [ "${#m0_pids[@]}" -gt 0 ];
then
   top -b -n 1 "${m0_pids[@]/#/-p}" > $m0d_pids
   args+=($m0d_pids)
fi

## Collect LDAP data
echo "Collecting ldap data"
mkdir -p $ldap_dir
if [[ $? != 0 || -z "$sgiamadminpwd" ]]
then
    echo "ERROR: ldap admin password: '$sgiamadminpwd' is not correct, skipping collection of ldap data."
else
    # Run ldap commands
    ldap_endpoint_key=$(s3confstore "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml" getkey --key="CONFIG>CONFSTORE_S3_OPENLDAP_ENDPOINTS")
    ldapendpoint=$(s3confstore "yaml://$confstore_url" getkey --key="$ldap_endpoint_key")
    # o/p of ldapendpoint -> ['ldap://127.0.0.1:389', 'ssl://127.0.0.1:636']
    ldapendpoint=${ldapendpoint// /} # remove spaces
    ldapendpoint=${ldapendpoint:1:-1} # remove [ and ]
    IFS=',' # comma seperator
    for endpoint in $ldapendpoint
    do
        endpoint=${endpoint:1:-1} # Remove ' and '
        echo "endpoint: $endpoint" # ldap://127.0.0.1:389 or ssl://127.0.0.1:636
        # TODO
        # Need to verify the endpoint works with ldapsearch as is or not
        # Also, need to verify on container
        echo "ldapsearch -b "cn=config" -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" -H $endpoint" >> "$ldap_config"
        ldapsearch -b "cn=config" -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" -H $endpoint  >> "$ldap_config"  2>&1
        echo "ldapsearch -s base -b "cn=subschema" objectclasses -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" -H $endpoint" >> "$ldap_subschema"
        ldapsearch -s base -b "cn=subschema" objectclasses -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" -H $endpoint >> "$ldap_subschema"  2>&1
        echo "ldapsearch -b "ou=accounts,dc=s3,dc=seagate,dc=com" -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" "objectClass=Account" -H $endpoint -LLL ldapentrycount" >> "$ldap_accounts"
        ldapsearch -b "ou=accounts,dc=s3,dc=seagate,dc=com" -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" "objectClass=Account" -H $endpoint -LLL ldapentrycount >> "$ldap_accounts" 2>&1
        echo "ldapsearch -b "ou=accounts,dc=s3,dc=seagate,dc=com" -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" "objectClass=iamUser" -H $endpoint -LLL ldapentrycount" >> "$ldap_users"
        ldapsearch -b "ou=accounts,dc=s3,dc=seagate,dc=com" -x -w "$sgiamadminpwd" -D "cn=sgiamadmin,dc=seagate,dc=com" "objectClass=iamUser" -H $endpoint -LLL ldapentrycount >> "$ldap_users"  2>&1
    done
    if [ -f "$ldap_config" ];
    then
        args+=($ldap_config)
    fi

    if [ -f "$ldap_subschema" ];
    then
        args+=($ldap_subschema)
    fi

    if [ -f "$ldap_accounts" ];
    then
        args+=($ldap_accounts)
    fi

    if [ -f "$ldap_users" ];
    then
        args+=($ldap_users)
    fi
fi

# Clean up temp files
cleanup_tmp_files(){
rm -rf "$tmp_dir"
}

## 2. Build tar.gz file with bundleid at bundle_path location
# Create folder with component name at given destination

mkdir -p $s3_bundle_location

# Build tar file
echo "Generating tar..."

#tar -cf - $args --warning=no-file-changed 2>/dev/null | xz -1e --thread=0 > $s3_bundle_location/$bundle_name 2>/dev/null

echo $args
tar -cvJf $s3_bundle_location/$bundle_name "${args[@]}" --warning=no-file-changed

# Clean up temp files
cleanup_tmp_files
echo "S3 support bundle generated successfully at $s3_bundle_location/$bundle_name !!!"
