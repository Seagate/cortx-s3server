#!/bin/sh
# Script to build third party libs
set -xe

usage() {
  echo 'Usage: ./build_thirdparty.sh [--no-motr-build][--help]'
  echo 'Optional params as below:'
  echo '          --no-motr-build  : If this option is set, then do not build motr.'
  echo '                             Default is (false). true = skip motr build.'
  echo '          --help (-h)      : Display help'
}

# read the options
OPTS=`getopt -o h --long no-motr-build,help -n 'build_thirdparty.sh' -- "$@"`

eval set -- "$OPTS"

no_motr_build=0
# extract options and their arguments into variables.
while true; do
  case "$1" in
    --no-motr-build) no_motr_build=1; shift ;;
    -h|--help) usage; exit 0;;
    --) shift; break ;;
    *) echo "Internal error!" ; exit 1 ;;
  esac
done

# Always refresh to ensure thirdparty patches can be applied.
./refresh_thirdparty.sh

# Before we build s3, get all dependencies built.
S3_SRC_FOLDER=`pwd`
cd third_party
./build_libevent.sh
./build_libevhtp.sh
./setup_jsoncpp.sh

if [ $no_motr_build -eq 0 ]
then
  ./build_motr.sh
fi

cd $S3_SRC_FOLDER
