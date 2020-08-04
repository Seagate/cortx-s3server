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

#Script to run UT's of s3recovery tool
set -e

abort()
{
    echo >&2 '
***************
*** FAILED ***
***************
'
    echo "Error encountered. Exiting unit test runs prematurely..." >&2
    trap : 0
    exit 1
}
trap 'abort' 0

printf "\nRunning s3recovery tool UT's...\n"

SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

#Update python path to source modules and run unit tests.
PYTHONPATH=${PYTHONPATH}:${SCRIPT_DIR}/.. python36 -m pytest $SCRIPT_DIR/../ut/*.py -v

echo "s3recovery tool UT's runs successfully"

trap : 0

