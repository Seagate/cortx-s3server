#!/bin/sh
# Script to run UTs & STs
set -e

usage() {
  echo 'Usage: ./runalltest.sh [--no-mero-rpm][--no-ut-run][--no-st-run][--no-ossperf-run]'
  echo '                       [--help]'
  echo 'Optional params as below:'
  echo '          --no-mero-rpm    : Use mero libs from source code (third_party/mero)'
  echo '                             Default is (false) i.e. use mero libs from pre-installed'
  echo '                             mero rpm location (/usr/lib64)'
  echo '          --no-ut-run      : Do not run UTs, Default (false)'
  echo '          --no-st-run      : Do not run STs, Default (false)'
  echo '          --no-ossperf-run : Do not run parallel/sequential perf tests by ossperf tool, Default (false)'
  echo '          --help (-h)      : Display help'
}

# read the options
OPTS=`getopt -o h --long no-mero-rpm,no-ut-run,no-st-run,no-ossperf-run,help -n 'runalltest.sh' -- "$@"`

eval set -- "$OPTS"

no_mero_rpm=0
no_ut_run=0
no_st_run=0
no_ossperf_run=0
# extract options and their arguments into variables.
while true; do
  case "$1" in
    --no-mero-rpm) no_mero_rpm=1; shift ;;
    --no-ut-run) no_ut_run=1; shift ;;
    --no-st-run) no_st_run=1; shift ;;
    --no-ossperf-run) no_ossperf_run=1; shift ;;
    -h|--help) usage; exit 0;;
    --) shift; break ;;
    *) echo "Internal error!" ; exit 1 ;;
  esac
done

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

if [ $no_mero_rpm -eq 1 ]
then
# use mero libs from source code
export LD_LIBRARY_PATH="$(pwd)/third_party/mero/mero/.libs/:"\
"$(pwd)/third_party/mero/extra-libs/gf-complete/src/.libs/"
fi

WORKING_DIR=`pwd`

if [ $no_ut_run -eq 0 ]
then
  UT_BIN=`pwd`/bazel-bin/s3ut
  UT_DEATHTESTS_BIN=`pwd`/bazel-bin/s3utdeathtests
  UT_MEMPOOL_BIN=`pwd`/bazel-bin/s3mempoolut
  UT_MEMPOOLMGR_BIN=`pwd`/bazel-bin/s3mempoolmgrut

  printf "\nCheck s3ut..."
  type  $UT_BIN >/dev/null
  printf "OK \n"

  $UT_BIN 2>&1

  printf "\nCheck s3mempoolmgrut..."
  type  $UT_MEMPOOLMGR_BIN >/dev/null
  printf "OK \n"

  $UT_MEMPOOLMGR_BIN 2>&1

  printf "\nCheck s3utdeathtests..."
  type  $UT_DEATHTESTS_BIN >/dev/null
  printf "OK \n"

  $UT_DEATHTESTS_BIN 2>&1

  printf "\nCheck s3mempoolut..."
  type $UT_MEMPOOL_BIN >/dev/null
  printf "OK \n"

  $UT_MEMPOOL_BIN 2>&1
fi

if [ $no_st_run -eq 0 ]
then
  CLITEST_SRC=`pwd`/st/clitests
  cd $CLITEST_SRC

  sh ./runallsystest.sh
fi

if [ $no_ossperf_run -eq 0 ]
then
  PERF_SRC=$WORKING_DIR/perf
  cd $PERF_SRC

  if [ -d "testfiles" ]
  then
    rm -rf testfiles
  fi

  echo "ossperf tests..."
  echo "Parallel worload of 5 files each of size 5000 bytes"
  ossperf.sh -n 5 -s 5000 -b seagatebucket -c ../st/clitests/virtualhoststyle.s3cfg -p 2>&1
  if [ $? -ne 0 ]; then
    echo "ossperf -- parallel workload test succeeded"
  fi
  echo "Sequential workload of 5 files each of size 5000 bytes"
  ossperf.sh -n 5 -s 5000 -b seagatebucket -c ../st/clitests/virtualhoststyle.s3cfg 2>&1
  if [ $? -ne 0 ]; then
     echo "ossperf -- sequential workload test succeeded"
  fi
  # Parallel multipart workload
  ossperf.sh -n 2 -s 18874368 -b seagatebucket -c ../st/clitests/virtualhoststyle.s3cfg 2>&1 -p
  if [ $? -ne 0 ]; then
     echo "ossperf --  parallel workload(Multipart) succeeded"
  fi
  # Sequential multipart workload
  ossperf.sh -n 2 -s 18874368 -b seagatebucket -c ../st/clitests/virtualhoststyle.s3cfg 2>&1
  if [ $? -ne 0 ]; then
     echo "ossperf -- sequential workload(Multipart) succeeded"
  fi
echo "*************************************************"
echo "*** System tests with ossperf Runs Successful ***"
echo "*************************************************"

fi

cd $WORKING_DIR
trap : 0
