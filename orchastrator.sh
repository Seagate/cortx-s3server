#!/bin/sh
set -x
USAGE=" Script will run motr recovery followed by s3 recovery.
USAGE: bash $(basename "$0") [--interactive]
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

motr_recovery_install_location=/opt/seagate/cortx/motr/
s3_recovery_install_location=/opt/seagate/cortx/s3/s3datarecovery/

cd $motr_recovery_install_location

./motr_data_recovery.sh || { echo "Motr recovery has failed. Run Orchestration again to recover data." && exit 1; }

cd $s3_recovery_install_location

if [ $reciprocative -eq 1 ]
then
  ./s3_data_recovery.sh --reciprocative || { echo "S3 data recovery has failed. Run only s3 recovery again" && exit 1; }
else
  ./s3_data_recovery.sh || { echo "S3 data recovery has failed. Run only s3 recovery again" && exit 1; }
fi
