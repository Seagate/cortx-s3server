#!/bin/sh
# Simple helper script to rebuild all s3 related binaries & install.
set -e

usage() {
  printf "%s\n%s\n%s\n%s\n\n%s\n\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n" \
    "   Usage: ./rebuildall.sh [--no-check-code][--no-clean-build][--no-s3ut-build]" \
    "                          [--no-s3server-build][--no-auth-build]" \
    "                          [--no-jclient-build][--no-jcloudclient-build]" \
    "                          [--no-install] [--help]" \
    "          Optional params as below:" \
    "          --no-check-code         : Do not check code for formatting style, Default (false)" \
    "          --no-clean-build        : Do not clean before build, Default (false)" \
    "                                    Use this option for incremental build." \
    "                                    This option is not recommended, use with caution." \
    "          --no-s3ut-build         : Do not build S3 UT, Default (false)" \
    "          --no-s3server-build     : Do not build S3 Server, Default (false)" \
    "          --no-auth-build         : Do not build Auth Server, Default (false)" \
    "          --no-jclient-build      : Do not build jclient, Default (false)" \
    "          --no-jcloudclient-build : Do not build jcloudclient, Default (false)" \
    "          --no-install            : Do not install binaries after build, Default (false)" \
    "          --help (-h)             : Display help"

}

# read the options
OPTS=`getopt -o h --long no-check-code,no-clean-build,no-s3ut-build,\
no-s3server-build,no-auth-build,no-jclient-build,no-jcloudclient-build,\
no-install,help -n 'rebuildall.sh' -- "$@"`

eval set -- "$OPTS"

no_check_code=0
no_clean_build=0
no_s3ut_build=0
no_s3server_build=0
no_auth_build=0
no_jclient_build=0
no_jcloudclient_build=0
no_install=0
# extract options and their arguments into variables.
while true; do
  case "$1" in
    --no-check-code) no_check_code=1; shift ;;
    --no-clean-build)no_clean_build=1; shift ;;
    --no-s3ut-build) no_s3ut_build=1; shift ;;
    --no-s3server-build) no_s3server_build=1; shift ;;
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

if [ $no_clean_build -eq 0 ]
then
  if [[ $no_s3ut_build -eq 0 || $no_s3server_build -eq 0 ]]
  then
    bazel clean
  fi
fi
if [ $no_s3ut_build -eq 0 ]
then
  bazel build //:s3ut --cxxopt="-std=c++11" --define MERO_SRC=`pwd`/../..
fi
if [ $no_s3server_build -eq 0 ]
then
  bazel build //:s3server --cxxopt="-std=c++11" --define MERO_SRC=`pwd`/../..
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
  sudo ./makeinstall
fi
