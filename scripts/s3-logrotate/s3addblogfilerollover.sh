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

# script is used to delete the old addb logs in /var/motr/<s3server-instance> directory
# argument1: <number of latest addb directories to retain>
# Default number of latest log files is 2
# ./s3addblogfilerollover.sh -n 2

usage() { echo "Usage: bash $(basename "$0")[--help|-h]
                   [-n addbDirCount]
Retain recent modified addb directories of given count.

where:
-n            number of latest addb directories to retain (Default count is 2)
--help|-h     display this help and exit" 1>&2; exit 1; }

# max addb files count in each s3 instance log directory
addb_dirs_max_count=2
s3server_config="/opt/seagate/cortx/s3/conf/s3config.yaml"
s3_daemon_working_dir=`cat $s3server_config | grep "S3_DAEMON_WORKING_DIR:" | cut -f2 -d: | sed -e 's/^[ \t]*//' -e 's/#.*//' -e 's/^[ \t]*"\(.*\)"[ \t]*$/\1/'`

while getopts ":n:" option; do
    case "${option}" in
        n)
            addb_dirs_max_count=${OPTARG}
            if [ -z ${addb_dirs_max_count} ]
            then
              usage
            fi
            ;;
        *)
            usage
            ;;
    esac
done

echo "Max addb dir count: $addb_dirs_max_count"
echo

echo "Rotating addb files in each $s3_daemon_working_dir<s3server-instance> directory"
if [[ -n "$s3_daemon_working_dir" && -d "$s3_daemon_working_dir" ]]
then
   # get s3server instance directories from /var/motr
   s3_dirs=`find $s3_daemon_working_dir -maxdepth 1 -type d -name "s3server-*"`
   for s3_instance_dir in $s3_dirs
   do
     # Find addb files
     addb_dirs=`find $s3_instance_dir -maxdepth 1 -type d -name "addb_*"`
     addb_dirs_count=`echo "$addb_dirs" | grep -v "^$" | wc -l`
     echo "## found $addb_dirs_count directories(s) in log directory($s3_instance_dir) ##"
     # check log files count is greater than max log file count or not
     if [ $addb_dirs_count -gt $addb_dirs_max_count ]
     then
        # get files sort by date - oldest will come on top
        remove_dir_count=`expr $addb_dirs_count - $addb_dirs_max_count`
        if [ $remove_dir_count -gt 0 ]
        then
            echo "## ($remove_dir_count) addb directories(s) can be removed from log directory($s3_instance_dir) ##"
            # get the files sorted by time modified (most recently modified comes last), that is oldest files will come on top
            dirs_to_remove=`ls -tr "$s3_instance_dir" | grep addb_ | head -n $remove_dir_count`
            for dir in $dirs_to_remove
            do
              rm -rf "$s3_instance_dir/$dir"
              echo Deleting "$s3_instance_dir/$dir"
            done
            echo "## deleted ($remove_dir_count) addb directories(s) from log directory($s3_instance_dir) ##"
        else
            echo "## No addb directories to remove ##"
        fi
     else
         echo "## No addb directories to remove ##"
     fi
   done
else
    echo "## No addb directories to remove ##"
fi

