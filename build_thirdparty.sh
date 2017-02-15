#!/bin/sh
# Script to build third party libs
set -e

usage() {
  echo 'Usage: ./build_thirdparty.sh [--no-mero-rpm][--help]'
  echo 'Optional params as below:'
  echo '          --no-mero-rpm    : Use mero libs from source code (third_party/mero) location'
  echo '                             If this option is set, then build mero from source code.'
  echo '                             Default is (false) i.e. use mero libs from pre-installed'
  echo '                             mero rpm location (/usr/lib64), i.e skip mero build.'
  echo '          --help (-h)      : Display help'
}

# read the options
OPTS=`getopt -o h --long no-mero-rpm,help -n 'build_thirdparty.sh' -- "$@"`

eval set -- "$OPTS"

no_mero_rpm=0
# extract options and their arguments into variables.
while true; do
  case "$1" in
    --no-mero-rpm) no_mero_rpm=1; shift ;;
    -h|--help) usage; exit 0;;
    --) shift; break ;;
    *) echo "Internal error!" ; exit 1 ;;
  esac
done

set -x

# Before we build s3, get all dependencies built.
S3_SRC_FOLDER=`pwd`
cd third_party
./build_libevent.sh
./build_libevhtp.sh
./setup_jsoncpp.sh
./build_libxml2.sh
./build_gtest.sh
./build_gmock.sh
./build_yamlcpp.sh
./build_gflags.sh
./build_glog.sh
if [ $no_mero_rpm -eq 1 ]
then
  ./build_mero.sh
fi

cd $S3_SRC_FOLDER

# We copy generated source to server for inline compilation.
cp third_party/jsoncpp/dist/jsoncpp.cpp server/jsoncpp.cc
