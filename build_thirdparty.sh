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

# Script to build third party libs
set -xe

usage() {
  echo 'Usage: ./build_thirdparty.sh [--no-motr-build][--help]'
  echo 'Optional params as below:'
  echo '          --no-motr-build  : If this option is set, then do not build motr.'
  echo '                             Default is (false). true = skip motr build.'
  echo '          --help (-h)      : Display help'
}

# read the options
OPTS=`getopt -o h --long no-motr-build,help -n 'build_thirdparty.sh' -- "$@"`

eval set -- "$OPTS"

no_motr_build=0
# extract options and their arguments into variables.
while true; do
  case "$1" in
    --no-motr-build) no_motr_build=1; shift ;;
    -h|--help) usage; exit 0;;
    --) shift; break ;;
    *) echo "Internal error!" ; exit 1 ;;
  esac
done

# Always refresh to ensure thirdparty patches can be applied.
./refresh_thirdparty.sh

# Before we build s3, get all dependencies built.
S3_SRC_FOLDER=`pwd`
cd third_party
./build_libevent.sh
./build_libevhtp.sh
./setup_jsoncpp.sh

if [ $no_motr_build -eq 0 ]
then
  ./build_motr.sh
fi

cd $S3_SRC_FOLDER
