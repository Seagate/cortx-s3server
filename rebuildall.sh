#!/bin/sh
# Simple helper script to rebuild all s3 related binaries & install.
set -e

usage() {
  echo 'Usage: ./rebuildall.sh [--no-mero-rpm][--no-mero-build][--no-thirdparty-build][--no-check-code]'
  echo '                       [--no-clean-build][--no-s3ut-build][--no-s3mempoolut-build]'
  echo '                       [--no-s3server-build][--no-cloviskvscli-build][--no-auth-build]'
  echo '                       [--no-jclient-build][--no-jcloudclient-build][--no-install][--help]'
  echo 'Optional params as below:'
  echo '          --no-mero-rpm           : Use mero libs from source code (third_party/mero) location'
  echo '                                    Default is (false) i.e. use mero libs from pre-installed'
  echo '                                    mero rpm location (/usr/lib64)'
  echo '          --no-mero-build         : Do not build mero, Default (false)'
  echo '          --no-thirdparty-build   : Do not build third party libs, Default (false)'
  echo '          --no-check-code         : Do not check code for formatting style, Default (false)'
  echo '          --no-clean-build        : Do not clean before build, Default (false)'
  echo '                                    Use this option for incremental build.'
  echo '                                    This option is not recommended, use with caution.'
  echo '          --no-s3ut-build         : Do not build S3 UT, Default (false)'
  echo '          --no-s3mempoolut-build  : Do not build Memory pool UT, Default (false)'
  echo '          --no-s3server-build     : Do not build S3 Server, Default (false)'
  echo '          --no-cloviskvscli-build : Do not build cloviskvscli tool, Default (false)'
  echo '          --no-auth-build         : Do not build Auth Server, Default (false)'
  echo '          --no-jclient-build      : Do not build jclient, Default (false)'
  echo '          --no-jcloudclient-build : Do not build jcloudclient, Default (false)'
  echo '          --no-install            : Do not install binaries after build, Default (false)'
  echo '          --help (-h)             : Display help'
}

# read the options
OPTS=`getopt -o h --long no-mero-rpm,no-mero-build,no-thirdparty-build,no-check-code,no-clean-build,\
no-s3ut-build,no-s3mempoolut-build,no-s3server-build,no-cloviskvscli-build,no-auth-build,\
no-jclient-build,no-jcloudclient-build,no-install,help -n 'rebuildall.sh' -- "$@"`

eval set -- "$OPTS"

no_mero_rpm=0
no_mero_build=0
no_thirdparty_build=0
no_check_code=0
no_clean_build=0
no_s3ut_build=0
no_s3mempoolut_build=0
no_s3server_build=0
no_cloviskvscli_build=0
no_auth_build=0
no_jclient_build=0
no_jcloudclient_build=0
no_install=0
# extract options and their arguments into variables.
while true; do
  case "$1" in
    --no-mero-rpm) no_mero_rpm=1; shift ;;
    --no-thirdparty-build) no_thirdparty_build=1; shift ;;
    --no-mero-build) no_mero_build=1; shift ;;
    --no-check-code) no_check_code=1; shift ;;
    --no-clean-build)no_clean_build=1; shift ;;
    --no-s3ut-build) no_s3ut_build=1; shift ;;
    --no-s3mempoolut-build) no_s3mempoolut_build=1; shift ;;
    --no-s3server-build) no_s3server_build=1; shift ;;
    --no-cloviskvscli-build) no_cloviskvscli_build=1; shift ;;
    --no-auth-build) no_auth_build=1; shift ;;
    --no-jclient-build) no_jclient_build=1; shift ;;
    --no-jcloudclient-build) no_jcloudclient_build=1; shift ;;
    --no-install) no_install=1; shift ;;
    -h|--help) usage; exit 0;;
    --) shift; break ;;
    *) echo "Internal error!" ; exit 1 ;;
  esac
done

set -x


if [ $no_check_code -eq 0 ]
then
  ./checkcodeformat.sh
fi

# Used to store third_party build artifacts
SEAGATE_SRC=/usr/local/seagate
CURRENT_USER=`whoami`
USE_SUDO=
if [[ $EUID -ne 0 ]]; then
   USE_SUDO=sudo
   command -v sudo || USE_SUDO=
fi
if [ $no_thirdparty_build -eq 0 ]
then
  $USE_SUDO rm -rf ${SEAGATE_SRC}
  $USE_SUDO mkdir -p ${SEAGATE_SRC} && $USE_SUDO chown ${CURRENT_USER}:${CURRENT_USER} ${SEAGATE_SRC}
  ./build_thirdparty.sh
else
  if [ ! -d ${SEAGATE_SRC} ]
  then
    echo "Third party sources should be rebuilt..."
    exit 1
  fi
fi

# Create symlinks to third_party files installed in SEAGATE_SRC
# since bazel does not seem to like absolute paths in BUILD file.
[ -e third_party/libevent/s3_dist ] || ln -s ${SEAGATE_SRC}/libevent third_party/libevent/s3_dist
[ -e third_party/libevhtp/s3_dist ] || ln -s ${SEAGATE_SRC}/libevhtp third_party/libevhtp/s3_dist
[ -e third_party/libxml2/s3_dist ] || ln -s ${SEAGATE_SRC}/libxml2 third_party/libxml2/s3_dist
[ -e third_party/googletest/s3_dist ] || ln -s ${SEAGATE_SRC}/googletest third_party/googletest/s3_dist
[ -e third_party/googlemock/s3_dist ] || ln -s ${SEAGATE_SRC}/googlemock third_party/googlemock/s3_dist
[ -e third_party/yaml-cpp/s3_dist ] || ln -s ${SEAGATE_SRC}/yaml-cpp third_party/yaml-cpp/s3_dist
[ -e third_party/gflags/s3_dist ] || ln -s ${SEAGATE_SRC}/gflags third_party/gflags/s3_dist
[ -e third_party/glog/s3_dist ] || ln -s ${SEAGATE_SRC}/glog third_party/glog/s3_dist
[ -e third_party/jsoncpp/s3_dist ] || ln -s ${SEAGATE_SRC}/jsoncpp third_party/jsoncpp/s3_dist

# We copy generated source to server for inline compilation.
cp ${SEAGATE_SRC}/jsoncpp/jsoncpp.cc server/jsoncpp.cc

if [ $no_mero_build -eq 0 ]
then
  cd third_party
  ./build_mero.sh
  cd ..
fi

if [ $no_mero_rpm -eq 0 ]
then
  # use mero libs from pre-installed mero rpm location
  MERO_INC_="MERO_INC=/usr/include/mero/"
  MERO_LIB_="MERO_LIB=/usr/lib64/"
  MERO_EXTRA_LIB_="MERO_EXTRA_LIB=/usr/lib64/"
else
  # use mero libs from source code
  MERO_INC_="MERO_INC=`pwd`/third_party/mero/"
  MERO_LIB_="MERO_LIB=`pwd`/third_party/mero/mero/.libs/"
  MERO_EXTRA_LIB_="MERO_EXTRA_LIB=`pwd`/third_party/mero/extra-libs/gf-complete/src/.libs/"
fi

if [ $no_clean_build -eq 0 ]
then
  if [[ $no_s3ut_build -eq 0   || \
      $no_s3server_build -eq 0 || \
      $no_cloviskvscli_build -eq 0 || \
      $no_s3mempoolut_build -eq 0 ]]
  then
    bazel clean
  fi
fi
if [ $no_s3ut_build -eq 0 ]
then
  bazel build //:s3ut --cxxopt="-std=c++11" --define $MERO_INC_ \
                      --define $MERO_LIB_ --define $MERO_EXTRA_LIB_

  bazel build //:s3utdeathtests --cxxopt="-std=c++11" --define $MERO_INC_ \
                                --define $MERO_LIB_ --define $MERO_EXTRA_LIB_
fi
if [ $no_s3mempoolut_build -eq 0 ]
then
  bazel build //:s3mempoolut --cxxopt="-std=c++11"
fi
if [ $no_s3server_build -eq 0 ]
then
  bazel build //:s3server --cxxopt="-std=c++11" --define $MERO_INC_ \
                          --define $MERO_LIB_ --define $MERO_EXTRA_LIB_
fi
if [ $no_cloviskvscli_build -eq 0 ]
then
  bazel build //:cloviskvscli --cxxopt="-std=c++11" --define $MERO_INC_ \
                              --define $MERO_LIB_ --define $MERO_EXTRA_LIB_
fi

if [ $no_auth_build -eq 0 ]
then
  cd auth
  if [ $no_clean_build -eq 0 ]
  then
    mvn clean
  fi
  mvn package
  cd -
fi

if [ $no_jclient_build -eq 0 ]
then
  cd auth-utils/jclient/
  if [ $no_clean_build -eq 0 ]
  then
    mvn clean
  fi
  mvn package
  cp target/jclient.jar ../../st/clitests/
  cd -
fi

if [ $no_jcloudclient_build -eq 0 ]
then
  cd auth-utils/jcloudclient
  if [ $no_clean_build -eq 0 ]
  then
    mvn clean
  fi
  mvn package
  cp target/jcloudclient.jar ../../st/clitests/
  cd -
fi

if [ $no_install -eq 0 ]
then
  # install with root privilege
  $USE_SUDO ./makeinstall
fi
