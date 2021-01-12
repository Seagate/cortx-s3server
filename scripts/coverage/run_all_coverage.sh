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

set -e
set +o posix

# variables
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
SRC_DIR="$(dirname "$(dirname "$SCRIPT_DIR" )")"
# Default codacy project token for cortx-s3server
CODACY_PROJECT_TOKEN=$1
# Default branch for uploading coverage report
BRANCH="main"

DES_DIR="$SRC_DIR/.code_coverage"

usage() {
    echo "Usage: $0  [-l <coverage output path>] [-t <codacy project token>] [-b <branch or commit id>]
    optional arguments:
        -l    Generate coverage reports in specified directory [default: $DES_DIR]
        -t    Provide CODACY_PROJECT_TOKEN
        -b    Provide specific git branch or commit uuid [default: $BRANCH]
        -h    Show this help message and exit" 1>&2;
    exit 1;
}

# Runs code coverage for all cpp, java and python source code.
run_coverage() {
    $SCRIPT_DIR/cpp_coverage.sh
    $SCRIPT_DIR/java_coverage.sh
    $SCRIPT_DIR/python_coverage.sh

    echo "Code Coverage Report for cpp, java and python is generated successfully!!"
}

upload_report() {
    export CODACY_PROJECT_TOKEN
    export CODACY_API_BASE_URL=https://api.codacy.com

    commit_id=$(git rev-parse $BRANCH)

    codacy_coverage_url="https://coverage.codacy.com/get.sh"

    # Here, the coverage report "s3server_coverage.info" holds coverage for
    # both CPP and C source code. However, for uploading it to codacy, particular
    # language needs to be specified.
    # Hence, same report is being upload first for CPP, then for C code.
    # Upload cpp coverage report
    bash <(curl -Ls $codacy_coverage_url) report \
    -l CPP -r $DES_DIR/s3server_cpp_coverage.info --commit-uuid $commit_id

    # Upload C coverage report
    bash <(curl -Ls $codacy_coverage_url) report \
    -l C -r $DES_DIR/s3server_cpp_coverage.info --commit-uuid $commit_id

    # Upload Java coverage report
    bash <(curl -Ls $codacy_coverage_url) report \
    -l Java -r $DES_DIR/auth/encryptcli_report.xml --partial --commit-uuid $commit_id
    bash <(curl -Ls $codacy_coverage_url) report \
    -l Java -r $DES_DIR/auth/encryptutil_report.xml --partial --commit-uuid $commit_id
    bash <(curl -Ls $codacy_coverage_url) report \
    -l Java -r $DES_DIR/auth/authserver_report.xml --partial --commit-uuid $commit_id
    bash <(curl -Ls $codacy_coverage_url) final --commit-uuid $commit_id

    # Upload Python coverage report
    bash <(curl -Ls $codacy_coverage_url) report \
    -l Python -r $DES_DIR/s3server_python_coverage.xml --commit-uuid $commit_id
}

############################# Main ################################

# read the options
while getopts "l:t:b:h" x; do
    case "${x}" in
        l)
            DES_DIR=${OPTARG}
            ;;
        t)
            CODACY_PROJECT_TOKEN=${OPTARG}
            ;;
        b)
            BRANCH=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

# Destination directory where the generated coverage reports are dumped.
export DES_DIR
mkdir -p $DES_DIR

# Running code coverage for all languages (CPP, Java, Python)
run_coverage

# Uploading the coverage reports to codacy
upload_report

echo "Code Coverage Report uploaded to codacy successfully!!"
