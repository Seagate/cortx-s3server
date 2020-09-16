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

# script is used to delete the old m0trace logs in /var/motr/<s3server-instance> directory
# script will retain first origional m0trace file along with recent modified files of given count and remove rest of m0trace files
# argument1: <number of latest m0trace files to retain>
# Default number of latest log files is 5
# ./s3m0tracelogfilerollover.sh -n 5

usage() { echo "Usage: bash $(basename "$0")[--help|-h]
                   [-n m0traceFileCount]
Retain recent modified files of given count and first generated m0trace and remove rest of m0trace files.

where:
-n            number of latest m0trace files to retain (Default count for m0trace files is 5)
--help|-h     display this help and exit" 1>&2; exit 1; }

# max m0trace files count in each s3 instance log directory
m0trace_files_max_count=5
s3server_config="/opt/seagate/cortx/s3/conf/s3config.yaml"
s3_daemon_working_dir=`cat $s3server_config | grep "S3_DAEMON_WORKING_DIR:" | cut -f2 -d: | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`

while getopts ":n:" option; do
    case "${option}" in
        n)
            m0trace_files_max_count=${OPTARG}
            if [ -z ${m0trace_files_max_count} ]
            then
              usage
            fi
            ;;
        *)
            usage
            ;;
    esac
done

echo "Max m0trace file count: $m0trace_files_max_count"
echo

echo "Rotating m0trace files in each $s3_daemon_working_dir<s3server-instance> directory"
if [[ -n "$s3_daemon_working_dir" && -d "$s3_daemon_working_dir" ]]
then
   # get s3server instance directories from /var/motr
   s3_dirs=`find $s3_daemon_working_dir -maxdepth 1 -type d -name "s3server-*"`
   for s3_instance_dir in $s3_dirs
   do
     # Find m0trace files
     m0trace_files=`find $s3_instance_dir -maxdepth 1 -type f -name "m0trace.*"`
     m0trace_files_count=`echo "$m0trace_files" | grep -v "^$" | wc -l`
     echo "## found $m0trace_files_count file(s) in log directory($s3_instance_dir) ##"
     # check log files count is greater than max log file count or not
     if [ $m0trace_files_count -gt $m0trace_files_max_count ]
     then
        # get files sort by date - oldest will come on top
        remove_file_count=`expr $m0trace_files_count - $m0trace_files_max_count`
        # remove oldest files except first generated m0trace e.g.5 recent + 1 original file
        remove_file_count=`expr $remove_file_count - 1`
        if [ $remove_file_count -gt 0 ]
        then
            echo "## ($remove_file_count) m0trace file(s) can be removed from log directory($s3_instance_dir) ##"
            # get the files sorted by time modified (most recently modified comes last), that is oldest files will come on top
            # ignore first file since we need to maintain first m0trace file using 'sed' command
            files_to_remove=`ls -tr "$s3_instance_dir" | grep m0trace |sed "1 d" | head -n $remove_file_count`
            for file in $files_to_remove
            do
              rm -f "$s3_instance_dir/$file"
            done
            echo "## deleted ($remove_file_count) m0trace file(s) from log directory($s3_instance_dir) ##"
        else
            echo "## No m0trace files to remove ##"
        fi
     else
         echo "## No m0trace files to remove ##"
     fi
   done
else
    echo "## No m0trace files to remove ##"
fi

