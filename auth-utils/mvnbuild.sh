#!/bin/sh -e
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


#Script to build Jclient & Jcloud client.

SRC_ROOT="$(dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd ) " )"
CWD=$(pwd)
USAGE="USAGE: bash $(basename "$0") [clean|package [--skip-tets]] [--help | -h]

where:
clean              clean previous build
package            build and package. --skip-tests will disable test run.
--help             display this help and exit"

if [ $# -ne 1 ]
   then
     echo "Invalid arguments passed"
     echo "$USAGE"
     exit 1
fi

case "$1" in
    clean )
       cd $SRC_ROOT/auth-utils/jclient
       mvn clean
       cd $SRC_ROOT/auth-utils/jcloudclient
       mvn clean
       cd $CWD
        ;;
    package )
       opts=""
       if [ "$2" = "--skip-tests" ]; then
         opts+=" -Dmaven.test.skip=true"
       fi
       cd $SRC_ROOT/auth-utils/jclient
       mvn package $opts
       cd $SRC_ROOT/auth-utils/jcloudclient
       mvn package $opts
       cd $CWD
        ;;
    --help | -h )
        echo "$USAGE"
        exit 1
        ;;
    * )
        echo "Invalid argument passed"
        echo "$USAGE"
        exit 1
        ;;
esac
