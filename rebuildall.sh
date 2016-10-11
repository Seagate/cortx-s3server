#!/bin/sh -x
# Simple helper script to rebuild all s3 related binaries & install.
set -e

./checkcodeformat.sh

bazel clean
bazel build //:s3ut --cxxopt="-std=c++11" --define MERO_SRC=`pwd`/../..
bazel build //:s3server --cxxopt="-std=c++11" --define MERO_SRC=`pwd`/../..

cd auth
mvn clean package
cd -

cd auth-utils/jclient/
mvn clean package
cd -
cp auth-utils/jclient/target/jclient.jar st/clitests/

cd auth-utils/jcloudclient
mvn clean package
cd -
cp auth-utils/jcloudclient/target/jcloudclient.jar st/clitests/

# install with root privilege
sudo ./makeinstall
