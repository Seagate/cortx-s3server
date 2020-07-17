#!/bin/sh
USAGE=" Script will run motr recovery followed by s3 recovery.
USAGE: bash $(basename "$0") [--interactive]
                             [--only_s3recovery]
where:
--interactive        Use interactive,If s3 recovery tool needs to be run in user input mode.
--only_s3recovery    Use only_s3recovery,If only s3 recovery tool has to be run.we won't run motr recovery forthis"

interactive=0
only_s3recovery=0

if [! $# -eq 0 ]
then
  while [ "$1" != "" ]; do
    case "$1" in
      --interactive ) interactive=1;
          ;;
      --only_s3recovery ) only_s3recovery=1;
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

if [ $only_s3recovery -eq 0 ]
then
  ./mero.sh || { echo "Motr recovery has failed.Run Orchestration again to recover data." && exit 1; }
fi

if [ $interactive -eq 1 ]
then
  ./s3_data_recovery.sh --interactive || { echo "S3 data recovery has failed.run only s3 recovery again" && exit 1; }
else
  ./s3_data_recovery.sh || { echo "S3 data recovery has failed.run only s3 recovery again" && exit 1; }
fi
