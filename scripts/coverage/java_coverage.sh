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

# This script helps to generate coverage report (.xml) for Java code(auth server).

set -e

# variables
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
	if [ -d "$DES_DIR/auth/" ]; then
		rm -rf $DES_DIR/auth/*
	fi
}

# Builds and generates coverage report
run_coverage() {
	pushd "$SRC_DIR/auth"
	# cleans previous build
	./mvnbuild.sh clean
	# runs build and generates coverage report
	./mvnbuild.sh package
	popd
}

check_report() {
	test -f "$SRC_DIR/auth/encryptcli/target/site/jacoco/jacoco.xml" || die "Failed to generate Coverage report for encryptcli."
	test -f "$SRC_DIR/auth/encryptutil/target/site/jacoco/jacoco.xml" || die "Failed to generate Coverage report for encryptutil."
	test -f "$SRC_DIR/auth/server/target/site/jacoco/jacoco.xml" || die "Failed to generate Coverage report for authserver."
}

############################# Main ################################

cleanup
run_coverage
check_report

# Copy reports to destination directory
mkdir -p $DES_DIR/auth
cp "$SRC_DIR/auth/encryptcli/target/site/jacoco/jacoco.xml" $DES_DIR/auth/encryptcli_report.xml
cp "$SRC_DIR/auth/encryptutil/target/site/jacoco/jacoco.xml" $DES_DIR/auth/encryptutil_report.xml
cp "$SRC_DIR/auth/server/target/site/jacoco/jacoco.xml" $DES_DIR/auth/authserver_report.xml

echo "Java Code Coverage Report generated successfully!!"
