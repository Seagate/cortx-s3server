#!/bin/sh

set -e
abort()
{
    echo >&2 '
***************
*** ABORTED ***
***************
'
    echo "Error encountered. Exiting test runs prematurely..." >&2
    trap : 0
    exit 1
}
trap 'abort' 0

WORKING_DIR=`pwd`
UT_BIN=`pwd`/bazel-bin/s3ut
UT_MEMPOOL_BIN=`pwd`/bazel-bin/s3mempoolut

printf "\nCheck s3ut..."
type  $UT_BIN >/dev/null
printf "OK \n"

$UT_BIN 2>&1

printf "\nCheck s3mempoolut..."
type $UT_MEMPOOL_BIN >/dev/null
printf "OK \n"

$UT_MEMPOOL_BIN 2>&1

CLITEST_SRC=`pwd`/st/clitests
cd $CLITEST_SRC

sh ./runallsystest.sh

cd $WORKING_DIR
trap : 0
