#!/bin/sh
set +e

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

# Help function which stdouts possible arguments for this script 
_help_function(){
   echo ""
   echo "Usage: $0 -c s3_test_config -i integration_type"
   echo -e "\t-c S3 test Config Path"
   echo -e "\t-v S3 Test version ('v2', 'v4', 'all')"
   echo -e "\t-i Integration type ('ceph', 'splunk')"
   exit 1 
}

# Read script variables from argument
while getopts ":c:v:i:" opt
do
   case "$opt" in
        c ) S3_TEST_CONF="$OPTARG" ;;
        v ) S3_TEST_VERSION="$OPTARG" ;;
        i ) S3_INTEGRATION_TYPE="$OPTARG" ;;
        ? ) helpFunction ;; # Print _help_function in case parameter is non-existent
   esac
done

# Test script parameters
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
S3_TEST_VERSION="${S3_TEST_VERSION:-all}"
S3_TEST_DATA_V2="${SCRIPT_DIR}/${S3_INTEGRATION_TYPE}/s3_v2_tests.txt"
S3_TEST_DATA_V4="${SCRIPT_DIR}/${S3_INTEGRATION_TYPE}/s3_v4_tests.txt"
S3_TEST_DIR="${SCRIPT_DIR}/s3-tests"

# Performs cleanup, checkout and bootsrap on s3-tests
_prepare() {

      # Validate conf location arguments 
      if [ -z "${S3_TEST_CONF}" ]; then
            echo "Error : Please provide test conf location"
            _help_function
            exit 1
      fi

      # validate integration type argument
      if [ -z "${S3_INTEGRATION_TYPE}" ] &&  [ "${S3_INTEGRATION_TYPE}" != "splunk" -o  "${S3_INTEGRATION_TYPE}" != "ceph" ] ; then
            echo "Error : Please provide validat integration type"
            _help_function
            exit 1
      fi

      # Cleanup old s3_tests checkout
      rm -rf "${S3_TEST_DIR}"

      # Create log dir
      mkdir -p ${S3_TEST_DIR}

      # Clone s3-tests repo and execute bootstrap script 
      pushd "${S3_TEST_DIR}"

            if [ "${S3_INTEGRATION_TYPE}" = "splunk" ]; then
                  git clone https://github.com/splunk/s3-tests .            
                  git checkout "3dc9362b1d322a59bd4e8f207d5a94070502b78b"
            elif [ "${S3_INTEGRATION_TYPE}" = "ceph" ]; then
                  git clone https://github.com/ceph/s3-tests .            
                  git checkout "6d8c0059db0ec0f3a523fbc4093a1fcb0213ac3d"
            else 
                  echo "Invalid Integration Type -${S3_INTEGRATION_TYPE}"
                  exit 1
            fi
            ./bootstrap
      popd
}

# Execute compatability test against s3.seagate.com
_execute_compatability_tets() {

      TEST_DATA="${1}"
      LOG_FILE="$(basename ${TEST_DATA} .txt).log"
      rm -rf "${LOG_FILE}"
      while read test_name; do
            S3TEST_CONF=${S3_TEST_CONF} ${S3_TEST_DIR}/virtualenv/bin/nosetests -v --collect-only $test_name  2>&1 | tee -a "${LOG_FILE}"
      done < "${TEST_DATA}"
}

echo "############# S3 Compatibility Test Started - ${S3_INTEGRATION_TYPE} ################"

echo "[ ${S3_INTEGRATION_TYPE} ] : Prepare : Cloning s3-test repo"
_prepare

# Set up Virtualenv
echo " [ ${S3_INTEGRATION_TYPE} ] : 1 Activating virtualenv"
source $S3_TEST_DIR/virtualenv/bin/activate

export PYTHONHTTPSVERIFY=0

# Executes V2 testes
if [ "${S3_TEST_VERSION}" = "all" -o "${S3_TEST_VERSION}" = "v2" ]; then
      
      echo "[ ${S3_INTEGRATION_TYPE} ] : 2. Running V2 tests"
      
      # Running test with aws v2signature
      export S3_USE_SIGV4=0

      _execute_compatability_tets "${S3_TEST_DATA_V2}"

      sleep 30s
fi

# Executes V4 tests
if [ "${S3_TEST_VERSION}" = "all" -o "${S3_TEST_VERSION}" = "v4" ]; then
      
      echo "[ ${S3_INTEGRATION_TYPE} ] : 2. Running V4 tests"
      
      # Running test with aws v2signature
      export S3_USE_SIGV4=1

      _execute_compatability_tets "${S3_TEST_DATA_V4}"

      sleep 30s
fi

# Disabling virtual env
deactivate

echo "[ ${S3_INTEGRATION_TYPE} ] : compatibility test completed!!!"