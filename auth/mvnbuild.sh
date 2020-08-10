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


#Script to build all Auth server modules

SRC_ROOT="$(dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd ) " )"

USAGE="USAGE: bash $(basename "$0") [clean|package|jacoco-report] [--help | -h]

where:
clean              clean previous build
package            build and package
jacoco-report      generate system test coverage report
--help             display this help and exit"

if [ -z $1 ]
   then
     echo "$USAGE"
     exit 1
fi
defaultpasswd=false
case "$1" in
    clean )
       cd $SRC_ROOT/auth/encryptutil
       mvn clean
       cd $SRC_ROOT/auth/encryptcli
       mvn clean
       cd $SRC_ROOT/auth/server
       mvn clean
        ;;
    package )
       cd $SRC_ROOT/auth/encryptutil
       mvn package
       mvn install:install-file -Dfile=target/AuthEncryptUtil-1.0-0.jar -DgroupId=com.seagates3 -DartifactId=AuthEncryptUtil -Dversion=1.0-0 -Dpackaging=jar
       cd $SRC_ROOT/auth/encryptcli
       mvn package
       cd $SRC_ROOT/auth/server
       mvn package
        ;;
    jacoco-report )
       cd $SRC_ROOT/auth/encryptutil
       mvn jacoco:report
       cd $SRC_ROOT/auth/encryptcli
       mvn jacoco:report
       cd $SRC_ROOT/auth/server
       mvn jacoco:report
        ;;
    --help | -h )
        echo "$USAGE"
        exit 1
        ;;
esac
