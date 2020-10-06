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

# This script is to make sure we have only fixed number of s3 support bundle files
# on the jenkins node, at any given time.

usage() { echo "Usage: bash $(basename "$0") [-n s3supportbundleFileCount] [--help|-h]

  Retain s3 support bundle files of the 'given' count, and unlink rest of the s3 support bundle files.

  where:
  -n            number of latest s3 support bundle files to retain (Default value: 10)
  --help|-h     display this help and exit" 1>&2; exit 1; 
}

max_s3support_bundle_files=10
s3_support_bundle_files_dir="/jenkins-job-logs/s3"

while getopts ":n:" option
do
  case "$option" in
    n)
      max_s3support_bundle_files=$OPTARG
      ;;
    *)
      usage
      ;;
  esac
done

if [[ -d "$s3_support_bundle_files_dir" &&  "$(ls -A $s3_support_bundle_files_dir)" ]]
then
  s3_support_bundle_count=$(find $s3_support_bundle_files_dir -maxdepth 1 -type f | wc -l)
  if [ $s3_support_bundle_count -gt $max_s3support_bundle_files ]
  then
    remove_file_count=$(expr $s3_support_bundle_count - $max_s3support_bundle_files)
    if [ $remove_file_count -gt 0 ]
    then
      s3_support_bundles_to_remove=$(ls -tr $s3_support_bundle_files_dir | head -n $remove_file_count)
      for file in $s3_support_bundles_to_remove
      do
        rm -f "$s3_support_bundle_files_dir/$file"
      done
      echo "## deleted $remove_file_count oldest s3 support bundles from directory: $s3_support_bundle_files_dir ##"
    fi
  else
    echo "s3 support bundle files are with-in allowed count limit: $max_s3support_bundle_files"
  fi
else
  echo "directory: $s3_support_bundle_files_dir does not exist or is empty, nothing to do."
fi
