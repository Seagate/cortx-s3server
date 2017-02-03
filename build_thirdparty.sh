#!/bin/sh -xe

# Before we build s3, get all dependencies built.
S3_SRC_FOLDER=`pwd`
cd third_party
./build_libevent.sh
./build_libevhtp.sh
./setup_jsoncpp.sh
./build_libxml2.sh
./build_gtest.sh
./build_gmock.sh
./setup_lustre_headers.sh
./build_yamlcpp.sh
./build_gflags.sh
./build_glog.sh
./build_mero.sh
#./build_nodejs.sh

cd $S3_SRC_FOLDER

# We copy generated source to server for inline compilation.
cp third_party/jsoncpp/dist/jsoncpp.cpp server/jsoncpp.cc
