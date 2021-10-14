#!/bin/sh
#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

# script is used to delete the old haproxy logs in <base_log_dir>/s3/<machine-id>/haproxy/ directory
# script will retain first origional haproxy log file along with recent modified files of given count and remove rest of haproxy files
# argument1: <number of latest haproxy log files to retain>
# Default number of latest log files is 5
# ./haproxylogfilerollover.sh -n 5

usage() { echo "Usage: bash $(basename "$0")[--help|-h]
                   [-n haproxyLogFileCount]
Retain recent modified files of given count and first generated haproxy log file and remove rest of haproxy log files.

where:
-n            number of latest haproxy log files to retain (Default count for haproxy log files is 5)
--help|-h     display this help and exit" 1>&2; exit 1; }

# max haproxy files count in each haproxy log directory
haproxy_log_files_max_count=5
haproxy_log_file=$(s3confstore "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml" getkey --key="S3_HAPROXY_INI_SYMLINK")
haproxy_log_file=$(readlink -f $haproxy_log_file)
#haproxy_config=$(s3confstore "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml" getkey --key="S3_HAPROXY_INI_SYMLINK")
#haproxy_log_file=$(s3confstore "properties://$haproxy_config" getkey --key="LOG_FILE")
echo "haproxy log file: $haproxy_log_file"

while getopts ":n:" option; do
    case "${option}" in
        n)
            haproxy_log_files_max_count=${OPTARG}
            if [ -z ${haproxy_log_files_max_count} ]
            then
              usage
            fi
            ;;
        *)
            usage
            ;;
    esac
done

echo "Max haproxy log file count: $haproxy_log_files_max_count"
echo "Rotating haproxy log files"

haproxy_log_dir=$(dirname $haproxy_log_file)
echo "haproxy dir: $haproxy_log_dir"

if [ -n "$haproxy_log_dir" ]
then
     # Find haproxy log files
     haproxy_log_files=`find $haproxy_log_dir -maxdepth 1 -type f -name "haproxy.*"`
     haproxy_log_files_count=`echo "$haproxy_log_files" | grep -v "^$" | wc -l`
     echo "## found $haproxy_log_files_count file(s) in log directory($haproxy_log_dir) ##"
     # check log files count is greater than max log file count or not
     if [ $haproxy_log_files_count -gt $haproxy_log_files_max_count ]
     then
         # get files sort by date - oldest will come on top
         remove_file_count=`expr $haproxy_log_files_count - $haproxy_log_files_max_count`
         # remove oldest files except first generated haproxy log e.g.5 recent + 1 original file
         remove_file_count=`expr $remove_file_count - 1`
         if [ $remove_file_count -gt 0 ]
         then
            echo "## ($remove_file_count) haproxy log file(s) can be removed from log directory($haproxy_log_dir) ##"
            # get the files sorted by time modified (most recently modified comes last), that is oldest files will come on top
            # ignore first file since we need to maintain first haproxy log file using 'sed' command
            files_to_remove=`ls -tr "$haproxy_log_dir" | grep haproxy |sed "1 d" | head -n $remove_file_count`
            for file in $files_to_remove
            do
              rm -f "$haproxy_log_dir/$file"
            done
            echo "## deleted ($remove_file_count) haproxy log file(s) from log directory($haproxy_log_dir) ##"

         else
            echo "## No haproxy log files to remove ##1"
         fi
     else
         echo "## No haproxy log files to remove ##2"
     fi
else
    echo "## No haproxy log files to remove ##3"
fi
