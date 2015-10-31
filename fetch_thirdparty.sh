#!/bin/sh -x

# Before we build s3, get all dependencies built.
S3_SRC_FOLDER=`pwd`
cd third_party
./setup_libevent.sh
./setup_libevhtp.sh
./setup_jsoncpp.sh
./setup_libxml2.sh
./setup_gtest.sh
./setup_lustre_headers.sh

# We copy generated source to server for inline compilation.
cp third_party/jsoncpp/dist/jsoncpp.cpp server/jsoncpp.cc

cd $S3_SRC_FOLDER
