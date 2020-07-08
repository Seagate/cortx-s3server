#!/bin/sh
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
printf $SCRIPT_PATH
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
printf $SCRIPT_DIR
#Update python path to source modules and run unit tests.

PYTHONPATH=${PYTHONPATH}:${SCRIPT_DIR}/.. python36 -m unittest ${SCRIPT_DIR}/../ut/*.py -v

echo "s3recovery tool UT's runs successfully"

trap : 0

