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

#!/bin/sh
USAGE=" Script will run motr recovery followed by s3 recovery.
USAGE: bash $(basename "$0") [--reciprocative]
where:
--reciprocative        Use reciprocative, If s3 recovery tool needs to be run in user input mode. "

reciprocative=0

if [ $# != 0 ]
then
  while [ "$1" != "" ]; do
    case "$1" in
      --reciprocative ) reciprocative=1;
          ;;
      --help | -h )
          echo "$USAGE"
          exit 1
          ;;
      * )
          echo "Invalid argument passed";
          echo "$USAGE"
          exit 1
          ;;
    esac
    shift
  done
fi

motr_recovery_install_location=/opt/seagate/cortx/motr/libexec/
s3_recovery_install_location=/opt/seagate/cortx/s3/s3datarecovery/

echo "Orchestration script has been launched on host:$HOSTNAME"

cd $motr_recovery_install_location

./motr_data_recovery.sh || { echo "Motr recovery has failed. Run Orchestration again to recover data." && exit 1; }

cd $s3_recovery_install_location

if [ $reciprocative -eq 1 ]
then
  ./s3_data_recovery.sh --reciprocative || { echo "S3 data recovery has failed. Run only s3 recovery again" && exit 1; }
else
  ./s3_data_recovery.sh || { echo "S3 data recovery has failed. Run only s3 recovery again" && exit 1; }
fi

echo "/=========                 Recovery completed                 ==========/"
echo "/=========  Please stop the cluster and reboot all the nodes  ==========/"
