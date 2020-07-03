#!/bin/sh

error_handler() {
  errorCode=$?
  rm -f m0trace*
  exit $errorCode
}

trap error_handler ERR
set -e

/opt/seagate/cortx/s3/bin/cloviskvscli $*

rm -f m0trace*
