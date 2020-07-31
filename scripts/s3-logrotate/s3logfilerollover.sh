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

# script is used to delete the old s3server logs
# script will retain recent modified files of given count based on severity and remove older files
# argument1: <number of latest log files to retain>
# Default number of latest log files is 5
# ./logrotate.sh 5

usage() { echo "Usage: bash $(basename "$0") [--help|-h]
                   [-n LogFileCount]
Retain recent modified files of given count based on severity and remove older s3 log files.

where:
-n            number of latest log files to retain (Default count for log files is 5)
--help|-h     display this help and exit" 1>&2; exit 1; }

# max log files count in each log directory
log_files_max_count=5
s3server_config="/opt/seagate/cortx/s3/conf/s3config.yaml"
# have severity entries, as s3sever logs are created based on severity.
log_severity="INFO WARNING ERROR FATAL"
# get log directory from s3 server config file
# TODO: have to check alternatives to get log dorectory
s3server_logdir=`cat /opt/seagate/cortx/s3/conf/s3config.yaml | grep "S3_LOG_DIR:" | cut -f2 -d: | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`

while getopts ":n:" option; do
    case "${option}" in
        n)
            log_files_max_count=${OPTARG}
            if [ -z ${log_files_max_count} ]
            then
              usage
            fi
            ;;
        *)
            usage
            ;;
    esac
done

echo "Max log file count: $log_files_max_count"
echo "Log file directory: $s3server_logdir"
echo

# check for log directory entries
if [ -n "$s3server_logdir" ]
then
 # get the log directory of each s3 instance
 log_dirs=`find $s3server_logdir -maxdepth 1 -type d`
 if [ -n "$log_dirs" ]
 then
  # iterate through all log directories of each s3 instance
  for log_dir in $log_dirs
  do
   for sev in $log_severity
   do
    # get the log file by severity, as the log file created based on severity
    log_files=`find $log_dir/*$sev* -type f`
    # get the no. of log file count
    log_files_count=`echo "$log_files" | grep -v "^$" | wc -l`
    echo "## found $log_files_count file(s) in log directory($log_dir) with severity($sev) ##"
    # check log files count is greater than max log file count or not
    if [ $log_files_count -gt $log_files_max_count ]
    then
     # get files sort by date - older will come on top
     # ignore sym files
     # remove older files
     remove_file_count=`expr $log_files_count - $log_files_max_count`
     echo "## ($remove_file_count) file(s) can be removed from log directory($log_dir) with severity($sev) ##"
     # get the files sorted by time modified (most recently modified comes last), that is older files comes first
     files_to_remove=`ls -tr $log_dir/*$sev* | head -n $remove_file_count`
     for file in $files_to_remove
     do
      # remove only if file not sym file
      if !  [ -L $file ]
      then
       rm -f "$file"
      fi
     done
    else
     echo "## No files to remove ##"
    fi
   done
  done
 fi
fi
