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
      echo -e " Usage:"
      echo -e "--------"
      echo ""
      echo -e "\t sh $0 -c=s3_test_config -i=integration_type"
      echo ""
      echo -e "\t -c   | --test_conf            = S3 test Config Path  -[ Required ]"
      echo -e "\t -i   | --integration_type     = Integration type ('ceph', 'splunk') - [ Required ]"
      echo -e "\t -v   | --test_version         = S3 Test version ('v2', 'v4', 'all') - [ Optional ] "
      echo -e "\t -tr  | --test_repo            = S3 Test repo url ('https://github.com/splunk/s3-tests', 'https://github.com/ceph/s3-tests') - [ Optional ] "
      echo -e "\t -trr | --test_repo_revision   = S3 Test repo Revision/Hash ('master', '#commit_hash#') - [ Optional ] "
      echo ""
      exit 1 
}

# Validate input arguments
_validate_input_args() {

      echo  -e "[ ${S3_INTEGRATION_TYPE} ] : Validate : Validating input arguments"
      
      # Validate conf location arguments 
      if [ -z "${S3_TEST_CONF}" ]; then
            echo -e "\t Error : Please provide test conf location"
            _help_function
            exit 1
      fi

      # validate integration type argument
      if [ -z "${S3_INTEGRATION_TYPE}" ] &&  [ "${S3_INTEGRATION_TYPE}" != "splunk" -o  "${S3_INTEGRATION_TYPE}" != "ceph" ] ; then
            echo -e "\t Error : Please provide valid integration type"
            _help_function
            exit 1
      fi

      echo "------------ Test Parameters ------------------------------------"
      echo -e "\t S3_TEST_CONF            = ${S3_TEST_CONF}"
      echo -e "\t S3_INTEGRATION_TYPE     = ${S3_INTEGRATION_TYPE}"
      echo -e "\t S3_TEST_VERSION         = ${S3_TEST_VERSION}"
      echo -e "\t S3_TEST_REPO            = ${S3_TEST_REPO}"
      echo -e "\t S3_TEST_REPO_REVISION   = ${S3_TEST_REPO_REVISION}"
      echo "---------------------------------------------------------------------"

}

# Performs cleanup, checkout and bootsrap on s3-tests
_prepare() {

      echo  -e "[ ${S3_INTEGRATION_TYPE} ] : Prepare : Cloning & bootsraping  s3-test repo"

      # Assign s3-test repo url and revision based on integration type
      if [ "${S3_INTEGRATION_TYPE}" = "splunk" ] ; then
            S3_TEST_REPO="${S3_TEST_REPO:-${S3_TEST_SPLUNK_REPO}}"
            S3_TEST_REPO_REVISION="${S3_TEST_REPO_REVISION:-${S3_TEST_SPLUNK_REPO_REVISION}}"
      elif [ "${S3_INTEGRATION_TYPE}" = "ceph" ] ; then
            S3_TEST_REPO="${S3_TEST_REPO:-${S3_TEST_CEPH_REPO}}"
            S3_TEST_REPO_REVISION="${S3_TEST_REPO_REVISION:-${S3_TEST_CEPH_REPO_REVISION}}"
      else
            echo -e "\t Error : Unknown integration. Please provide valid arguments"
            _help_function
      fi

      # Clean reports dir 
      rm -rf "${REPORTS_DIR}"
      mkdir -p "${REPORTS_DIR}"

      # Clone s3-tests repo and execute bootstrap script 
      rm -rf "${S3_TEST_DIR}"
      mkdir -p ${S3_TEST_DIR}

      pushd "${S3_TEST_DIR}"

            # Checkout s3-test repo
            git clone "${S3_TEST_REPO}" . 
            git checkout "${S3_TEST_REPO_REVISION}"

            # Test script customerization
            if [ "${S3_INTEGRATION_TYPE}" = "ceph" ] ; then
                  git apply "${S3_TEST_CEPH_PATH}"
            fi

            # boostrap s3-test
            ./bootstrap

      popd

}

# Execute compatability test against s3.seagate.com
_execute_compatability_tets() {

      echo -e "[ ${S3_INTEGRATION_TYPE} ] : Test Execution : Running tests - ${1}"

      TEST_DATA="${1}"
      LOG_FILE="$(basename ${TEST_DATA} .txt)"

      cp "${S3_TEST_CONF}" "${S3_TEST_DIR}/s3test.conf"
      pushd ${S3_TEST_DIR}

            echo "S3_USE_SIGV4 = ${S3_USE_SIGV4}"
            echo "-------------------------------"
            S3TEST_CONF="s3test.conf" ./virtualenv/bin/nosetests $(<${TEST_DATA}) --verbose --logging-level=DEBUG --with-xunit --failure-detail --xunit-file="${REPORTS_DIR}/${LOG_FILE}.xml"  2>&1 | tee -a "${REPORTS_DIR}/${LOG_FILE}.log"
      popd
}

# Test wrapper which handles virtual env and dependent env vars   
_run_test(){

      # Set up Virtualenv
      echo " [ ${S3_INTEGRATION_TYPE} ] : run test : Activating virtualenv"
      source $S3_TEST_DIR/virtualenv/bin/activate
      export PYTHONHTTPSVERIFY=0

      # Executes V2 testes
      if [ "${S3_TEST_VERSION}" = "all" -o "${S3_TEST_VERSION}" = "v2" ]; then
                  
            # Running test with aws v2signature
            unset S3_USE_SIGV4

            _execute_compatability_tets "${S3_TEST_DATA_V2}"

            sleep 30s
      fi

      # Executes V4 tests
      if [ "${S3_TEST_VERSION}" = "all" -o "${S3_TEST_VERSION}" = "v4" ]; then
            
            # Running test with aws v4signature
            export S3_USE_SIGV4=1

            _execute_compatability_tets "${S3_TEST_DATA_V4}"

            unset S3_USE_SIGV4

      fi

      # Disabling virtual env
      deactivate

}

# Read script variables from argument
while [ $# -gt 0 ]; do
  case "$1" in
    -c=* | --test_conf=*)
      S3_TEST_CONF="${1#*=}"
      ;;
    -i=* | --integration_type=*)
      S3_INTEGRATION_TYPE="${1#*=}"
      ;;
    -v=* | --test_version=*)
      S3_TEST_VERSION="${1#*=}"
      ;;
    -tr=* | --test_repo=*)
      S3_TEST_REPO="${1#*=}"
      ;;
    -trr=* | --test_repo_revision=*)
      S3_TEST_REPO_REVISION="${1#*=}"
      ;;
    *)
      _help_function ;; # Print _help_function in case parameter is non-existent
  esac
  shift
done

echo -e "\n\n ############# s3 compatibility test started - ${S3_INTEGRATION_TYPE} ################ \n\n"

# Test script parameters
REPORTS_DIR="$(pwd)/reports"
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
S3_TEST_VERSION="${S3_TEST_VERSION:-all}"
S3_TEST_DATA_V2="${SCRIPT_DIR}/${S3_INTEGRATION_TYPE}/s3_v2_tests.txt"
S3_TEST_DATA_V4="${SCRIPT_DIR}/${S3_INTEGRATION_TYPE}/s3_v4_tests.txt"
S3_TEST_DIR="${SCRIPT_DIR}/s3-tests"

# Splunk s3-test repo/revision [ Default ]
S3_TEST_SPLUNK_REPO="https://github.com/splunk/s3-tests"
S3_TEST_SPLUNK_REPO_REVISION="3dc9362b1d322a59bd4e8f207d5a94070502b78b"

# Ceph s3-test repo/revision [ Default ]
S3_TEST_CEPH_REPO="https://github.com/ceph/s3-tests"
S3_TEST_CEPH_REPO_REVISION="b1815c25dcf829b29cf8fc38b6cf83f040e3aa51"
S3_TEST_CEPH_PATH="${SCRIPT_DIR}/${S3_INTEGRATION_TYPE}/ceph_s3_test_6d8c005.patch"


# Validate input arguments
_validate_input_args

# Prepare environemnt for test
_prepare

# Execute compatability tests
_run_test

echo -e "\n\n ############# s3 compatibility test completed - ${S3_INTEGRATION_TYPE} ################ \n\n"

