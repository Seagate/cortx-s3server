#!/bin/sh
# Script to build third party libs
set -xe

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
./build_s3cmd.sh

cd $S3_SRC_FOLDER
