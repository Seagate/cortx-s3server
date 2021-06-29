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

# This script helps to generate coverage report (.xml) for python code ( s3backgrounddelete )

set -e

#variables
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
SRC_DIR="$(dirname "$(dirname "$SCRIPT_DIR" )")"

DES_DIR=${DES_DIR:-"$SRC_DIR/.code_coverage"}

die() {
	echo "${*}"
	exit 1
}

# Remove stale coverage reports
cleanup() {
	echo "Cleaning up old coverage reports."
	local file="$DES_DIR/s3server_python_coverage.xml"

	if [ -f "$file" ]; then
		rm -f $file
	fi
}

# Install any dependency rpms/packages if not present
check_prerequisite() {
	# install coverage.py package if not present
	if ! command -v coverage &>/dev/null; then
		pip3 install coverage
	fi
}

# Run UTs of s3backgrounddelete
run_ut() {
	echo "Executing UTs"
	$SRC_DIR/s3backgrounddelete/scripts/run_all_ut.sh
}

# Run the coverage.py over UTs and generate coverage report
run_coverage() {
  # Running coverage.py tool over UTs
  #pushd "$SRC_DIR/s3backgrounddelete"
  coverage run -m pytest $SRC_DIR/s3backgrounddelete/ut/cortx_s3_* $SRC_DIR/s3backgrounddelete/s3backgrounddelete/*.py

  coverage report -m $SRC_DIR/s3backgrounddelete/ut/cortx_s3_*  $SRC_DIR/s3backgrounddelete/s3backgrounddelete/*.py
  # Generate xml report
  coverage xml -o "$DES_DIR/s3server_python_coverage.xml"
  #popd
}

# Validate the report generated
check_report() {
	test -f "$DES_DIR/s3server_python_coverage.xml" || die "Failed to generate the Coverage report."
}

############################# Main ################################

cleanup
check_prerequisite
run_ut
run_coverage
check_report

echo "Python Code Coverage Report generated successfully!!"
