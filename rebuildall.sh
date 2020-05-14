#!/bin/sh
# Simple helper script to rebuild all s3 related binaries & install.
set -e

usage() {
  echo 'Usage: ./rebuildall.sh [--no-mero-rpm][--use-build-cache][--no-check-code]'
  echo '                       [--no-clean-build][--no-s3ut-build][--no-s3mempoolut-build][--no-s3mempoolmgrut-build]'
  echo '                       [--no-s3server-build][--no-cloviskvscli-build][--no-auth-build]'
  echo '                       [--no-jclient-build][--no-jcloudclient-build][--no-install][--help]'
  echo 'Optional params as below:'
  echo '          --no-mero-rpm              : Use mero libs from source code (third_party/mero) location'
  echo '                                       Default is (false) i.e. use mero libs from pre-installed'
  echo '                                       mero rpm location (/usr/lib64)'
  echo '          --use-build-cache          : Use build cache for third_party and mero, Default (false)'
  echo '                                      If cache is missing, third_party and mero will be rebuilt'
  echo '                                      Ensuring consistency of cache is responsibility of caller'
  echo '          --no-check-code            : Do not check code for formatting style, Default (false)'
  echo '          --no-clean-build           : Do not clean before build, Default (false)'
  echo '                                       Use this option for incremental build.'
  echo '                                       This option is not recommended, use with caution.'
  echo '          --no-s3ut-build            : Do not build S3 UT, Default (false)'
  echo '          --no-s3mempoolut-build     : Do not build Memory pool UT, Default (false)'
  echo '          --no-s3mempoolmgrut-build  : Do not build Memory pool Manager UT, Default (false)'
  echo '          --no-s3server-build        : Do not build S3 Server, Default (false)'
  echo '          --no-cloviskvscli-build    : Do not build cloviskvscli tool, Default (false)'
  echo '          --no-s3background-build    : Do not build s3background process, Default (false)'
  echo '          --no-s3addbplugin-build    : Do not build s3 addb plugin library, Default (false)'
  echo '          --no-auth-build            : Do not build Auth Server, Default (false)'
  echo '          --no-jclient-build         : Do not build jclient, Default (false)'
  echo '          --no-jcloudclient-build    : Do not build jcloudclient, Default (false)'
  echo '          --no-s3iamcli-build        : Do not build s3iamcli, Default (false)'
  echo '          --no-install               : Do not install binaries after build, Default (false)'
  echo '          --help (-h)                : Display help'
}

declare -a dev_includes_array
declare -a dev_lib_search_paths_array
declare -a rpm_includes_array
declare -a rpm_lib_search_paths_array

declare -a link_libs_array

# Example cmd run and output
# CMD: PKG_CONFIG_PATH=./third_party/mero pkg-config --cflags-only-I mero
# OUTPUT: -I/root/s3server/third_party/mero -I/root/s3server/third_party/mero/extra-libs/gf-complete/include -I/usr/src/lustre-client-2.12.3/libcfs/include \
# -I/usr/src/lustre-client-2.12.3/lnet/include -I/usr/src/lustre-client-2.12.3/lnet/include/uapi/linux -I/usr/src/lustre-client-2.12.3/lustre/include \
# -I/usr/src/lustre-client-2.12.3/lustre/include/uapi/linux
#
# CMD: PKG_CONFIG_PATH=./third_party/mero pkg-config --libs mero
# OUTPUT: -L/root/s3server/third_party/mero/mero/.libs -lmero
#
get_mero_pkg_config_dev() {
  s3_src_dir=$1
  includes=$(PKG_CONFIG_PATH=$s3_src_dir/third_party/mero pkg-config --cflags-only-I mero)
  for include in $includes
  do
    if [[ "$include" == *"third_party"* ]] # exclude other paths
    then
      inc_path=$(echo $include | sed 's/^.*third_party/\/third_party/')
      dev_includes_array+=( $inc_path )
    fi
  done
  #printf '%s\t\n' "${dev_includes_array[@]}"
  libs=$(PKG_CONFIG_PATH=$s3_src_dir/third_party/mero pkg-config --libs mero)
  for lib in $libs
  do
    if [[ "$lib" == *"third_party"* ]] # this is 'L' part
    then
      lib_path=$(echo $lib | sed 's/^.*third_party/\/third_party/')
      dev_lib_search_paths_array+=( $lib_path )
    elif [[ ${lib:0:2} == '-l' ]]
    then
      link_libs_array+=( $lib )
    fi
  done
}

# Example cmd run and output
# CMD: pkg-config --cflags-only-I mero
# OUTPUT: -I/usr/include/mero -I/usr/src/lustre-client-2.12.3/libcfs/include -I/usr/src/lustre-client-2.12.3/lnet/include \
# -I/usr/src/lustre-client-2.12.3/lnet/include/uapi/linux -I/usr/src/lustre-client-2.12.3/lustre/include \
# -I/usr/src/lustre-client-2.12.3/lustre/include/uapi/linux \
#
# CMD: pkg-config --libs mero
# OUTPUT: -lmero
get_mero_pkg_config_rpm() {
  includes=$(pkg-config --cflags-only-I mero)
  for include in $includes
  do
    if [[ "$include" != *"lustre-client"* ]] # exclude include paths from 'lustre-client*'
    then
      inc_path=${include#"-I"}
      rpm_includes_array+=( $inc_path )
    fi
  done
  #printf '%s\t\n' "${rpm_includes_array[@]}"

  libs=$(pkg-config --libs mero)
  for lib in $libs
  do
    if [[ ${lib:0:2} == '-L' ]] # this is 'L' part
    then
      rpm_lib_search_paths_array+=( $lib )
    elif [[ ${lib:0:2} == '-l' ]]
    then
      link_libs_array+=( $lib )
    fi
  done
}

# read the options
OPTS=`getopt -o h --long no-mero-rpm,use-build-cache,no-check-code,no-clean-build,\
no-s3ut-build,no-s3mempoolut-build,no-s3mempoolmgrut-build,no-s3server-build,\
no-cloviskvscli-build,no-s3background-build,no-s3addbplugin-build,no-auth-build,\
no-jclient-build,no-jcloudclient-build,no-s3iamcli-build,no-install,help -n 'rebuildall.sh' -- "$@"`

eval set -- "$OPTS"

no_mero_rpm=0
use_build_cache=0
no_check_code=0
no_clean_build=0
no_s3ut_build=0
no_s3mempoolut_build=0
no_s3mempoolmgrut_build=0
no_s3server_build=0
no_cloviskvscli_build=0
no_s3background_build=0
no_s3addbplugin_build=0
no_auth_build=0
no_jclient_build=0
no_jcloudclient_build=0
no_s3iamcli_build=0
no_install=0

# extract options and their arguments into variables.
while true; do
  case "$1" in
    --no-mero-rpm) no_mero_rpm=1; shift ;;
    --use-build-cache) use_build_cache=1; shift ;;
    --no-check-code) no_check_code=1; shift ;;
    --no-clean-build)no_clean_build=1; shift ;;
    --no-s3ut-build) no_s3ut_build=1; shift ;;
    --no-s3mempoolut-build) no_s3mempoolut_build=1; shift ;;
    --no-s3mempoolmgrut-build) no_s3mempoolmgrut_build=1; shift ;;
    --no-s3server-build) no_s3server_build=1; shift ;;
    --no-cloviskvscli-build) no_cloviskvscli_build=1; shift ;;
    --no-s3background-build) no_s3background_build=1; shift ;;
    --no-s3addbplugin-build) no_s3addbplugin_build=1; shift ;;
    --no-auth-build) no_auth_build=1; shift ;;
    --no-jclient-build) no_jclient_build=1; shift ;;
    --no-jcloudclient-build) no_jcloudclient_build=1; shift ;;
    --no-s3iamcli-build) no_s3iamcli_build=1; shift ;;
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
S3_SRC_DIR=`pwd`
BUILD_CACHE_DIR=$HOME/.seagate_src_cache

# Used to store mero include paths which are read from 'pkg-config --cflags'
mero_include_path="\"-I"
declare MERO_LINK_LIB_

# Build steps for third_party and mero
if [ $no_mero_rpm -eq 0 ]
then
  # RPM based build, build third_party except mero
  ./build_thirdparty.sh --no-mero-build
else
  if [ $use_build_cache -eq 0 ]
  then
    # Rebuild all third_party
    ./build_thirdparty.sh
  else
    # Use build cache
    if [ ! -d ${BUILD_CACHE_DIR} ]
    then
      # Rebuild all third_party
      ./build_thirdparty.sh

      # Copy to CACHE
      rm -rf ${BUILD_CACHE_DIR}
      mkdir -p ${BUILD_CACHE_DIR}

      echo "Sync third_party(,mero) binaries from third_party/"
      rsync -aW $S3_SRC_DIR/third_party/mero/ $BUILD_CACHE_DIR/mero
      cd $S3_SRC_DIR/third_party/mero/ && git rev-parse HEAD > $BUILD_CACHE_DIR/cached_mero.git.rev && cd -

      mkdir -p $BUILD_CACHE_DIR/libevent
      rsync -aW $S3_SRC_DIR/third_party/libevent/s3_dist $BUILD_CACHE_DIR/libevent
      cd $S3_SRC_DIR/third_party/libevent/ && git rev-parse HEAD > $BUILD_CACHE_DIR/cached_libevent.git.rev && cd -

      mkdir -p $BUILD_CACHE_DIR/libevhtp
      rsync -aW $S3_SRC_DIR/third_party/libevhtp/s3_dist $BUILD_CACHE_DIR/libevhtp
      cd $S3_SRC_DIR/third_party/libevhtp/ && git rev-parse HEAD > $BUILD_CACHE_DIR/cached_libevhtp.git.rev && cd -

      mkdir -p $BUILD_CACHE_DIR/jsoncpp
      rsync -aW $S3_SRC_DIR/third_party/jsoncpp/dist $BUILD_CACHE_DIR/jsoncpp
      cd $S3_SRC_DIR/third_party/jsoncpp/ && git rev-parse HEAD > $BUILD_CACHE_DIR/cached_jsoncpp.git.rev && cd -
    fi  # build cache not present
    # Copy from cache
    rsync -aW $BUILD_CACHE_DIR/ $S3_SRC_DIR/third_party/
  fi  # if [ $use_build_cache -eq 0 ]
fi  # if [ $no_mero_rpm -eq 0 ]

cp -f $S3_SRC_DIR/third_party/jsoncpp/dist/jsoncpp.cpp $S3_SRC_DIR/server/jsoncpp.cc

# Do we want a clean S3 build?
if [ $no_clean_build -eq 0 ]
then
  if [[ $no_s3ut_build -eq 0   || \
      $no_s3server_build -eq 0 || \
      $no_cloviskvscli_build -eq 0 || \
      $no_s3mempoolmgrut_build -eq 0 || \
      $no_s3mempoolut_build -eq 0 ]]
  then
    # bazel shutdown
    # bazel clean --expunge
  fi
fi
# Define the paths
if [ $no_mero_rpm -eq 1 ] # use mero libs from source code (built location or cache)
then
  MERO_INC_="MERO_INC=./third_party/mero/"

  # set mero_include_path for 'copts' in BUILD file
  get_mero_pkg_config_dev $S3_SRC_DIR
  for path in "${dev_includes_array[@]}"
  do
    mero_include_path=$mero_include_path"."$path"\", \"-I"
  done
  # remove last ', "-I' # mero_include_path='"-I./third_party/mero", "-I./third_party/mero/extra-libs/gf-complete/include", \"-I'
  mero_include_path=${mero_include_path%", \"-I"}

  MERO_LIB_="MERO_LIB=."
  for lib_path in "${dev_lib_search_paths_array[@]}"
  do
    MERO_LIB_=$MERO_LIB_$lib_path"\", -L."  # '-L' is being appended at first index in 'BUILD' file itself
  done
  # remove last '\", -L.'
  MERO_LIB_=${MERO_LIB_%"\", -L."}

  for lib in "${link_libs_array[@]}"
  do
    MERO_LINK_LIB_=$lib" "
  done
  # remove last white space
  MERO_LINK_LIB_=${MERO_LINK_LIB_%" "}

  MERO_HELPERS_LIB_="MERO_HELPERS_LIB=./third_party/mero/helpers/.libs/"
else
  # use mero libs from pre-installed mero rpm location
  get_mero_pkg_config_rpm
  for path in "${rpm_includes_array[@]}"
  do
    mero_include_path=$mero_include_path$path"\", \"-I"
  done
  # remove last ', "-I'
  mero_include_path=${mero_include_path%", \"-I"}

  MERO_INC_="MERO_INC=/usr/include/mero/"
  if [ ${#rpm_lib_search_paths_array[@]} -eq 0 ]
  then
    MERO_LIB_="MERO_LIB=/usr/lib64/"
  else
    MERO_LIB_="MERO_LIB=."
    for lib_path in "${rpm_lib_search_paths_array[@]}"
    do
      MERO_LIB_=$MERO_LIB_$lib_path"\", -L."  # '-L' is being appended at first index in 'BUILD' file itself
    done
    # remove last '\", -L.'
    MERO_LIB_=${MERO_LIB_%"\", -L."}
  fi

  MERO_HELPERS_LIB_="MERO_HELPERS_LIB=/usr/lib64/"

  for lib in "${link_libs_array[@]}"
  do
    MERO_LINK_LIB_=$lib" "
  done
  # remove last white space
  MERO_LINK_LIB_=${MERO_LINK_LIB_%" "}
fi

# set mero library search path in 'BUILD' file
sed -i 's|MERO_DYNAMIC_INCLUDES|'"$mero_include_path"'|g' BUILD

# set mero link library in 'BUILD' file
sed -i 's/MERO_LINK_LIB/'"$MERO_LINK_LIB_"'/g' BUILD

if [ $no_s3ut_build -eq 0 ]
then
  bazel build //:s3ut --cxxopt="-std=c++11" --define $MERO_INC_ \
                      --define $MERO_LIB_ --define $MERO_HELPERS_LIB_ \
                      --spawn_strategy=standalone \
                      --strip=never

  bazel build //:s3utdeathtests --cxxopt="-std=c++11" --define $MERO_INC_ \
                                --define $MERO_LIB_ --define $MERO_HELPERS_LIB_ \
                                --spawn_strategy=standalone \
                                --strip=never
fi

if [ $no_s3mempoolut_build -eq 0 ]
then
  bazel build //:s3mempoolut --cxxopt="-std=c++11" --spawn_strategy=standalone \
                             --strip=never
fi

if [ $no_s3mempoolmgrut_build -eq 0 ]
then
  bazel build //:s3mempoolmgrut --cxxopt="-std=c++11" --define $MERO_INC_ \
                      --define $MERO_LIB_ --define $MERO_HELPERS_LIB_ \
                      --spawn_strategy=standalone \
                      --strip=never
fi

assert_addb_plugin_autogenerated_sources_are_correct() {
  cd server
  ./addb-codegen.py
  if test -n "`git diff s3_addb_*_auto*`"; then
    echo "ERROR (FATAL): There are changes in list of action classes!" >&2
    echo "You need to re-generate addb files.  To do that," >&2
    echo "cd to server/ folder and run addb-codegen.py script." >&2
    echo "Make sure to save your changes to git." >&2
    exit 1
  fi
  cd ..
}

if [ $no_s3server_build -eq 0 ]
then
  assert_addb_plugin_autogenerated_sources_are_correct
  bazel build //:s3server --cxxopt="-std=c++11" --define $MERO_INC_ \
                          --define $MERO_LIB_ --define $MERO_HELPERS_LIB_ \
                          --spawn_strategy=standalone \
                          --strip=never
fi

if [ $no_s3addbplugin_build -eq 0 ]
then
  assert_addb_plugin_autogenerated_sources_are_correct
  bazel build //:s3addbplugin --define $MERO_INC_ \
                              --define $MERO_LIB_ --define $MERO_HELPERS_LIB_ \
                              --spawn_strategy=standalone \
                              --strip=never
fi

if [ $no_cloviskvscli_build -eq 0 ]
then
  bazel build //:cloviskvscli --cxxopt="-std=c++11" --define $MERO_INC_ \
                              --define $MERO_LIB_ --define $MERO_HELPERS_LIB_ \
                              --spawn_strategy=standalone \
                              --strip=never
fi

# restore BUILD file
sed -i 's|'"$mero_include_path"'|MERO_DYNAMIC_INCLUDES|g' BUILD
sed -i 's/'"$MERO_LINK_LIB_ "'/MERO_LINK_LIB /g' BUILD

# Just to free up resources
bazel shutdown

if [ $no_auth_build -eq 0 ]
then
  cd auth
  if [ $no_clean_build -eq 0 ]
  then
    ./mvnbuild.sh clean
  fi
  ./mvnbuild.sh package
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
  cp target/classes/jclient.properties ../../st/clitests/
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
  cp target/classes/jcloud.properties ../../st/clitests/
  cd -
fi

if [ $no_install -eq 0 ]
then
  if [[ $EUID -ne 0 ]]; then
    command -v sudo
    if [ $? -ne 0 ]
    then
      echo "sudo required to run makeinstall"
      exit 1
    else
      sudo ./makeinstall
    fi
  else
    ./makeinstall
  fi
fi

if [ $no_mero_rpm -eq 1 ]
then
  if [ $no_s3background_build -eq 0 ]
  then
    cd s3backgrounddelete
    if [ $no_clean_build -eq 0 ]
    then
      python36 setup.py install --force
    else
      python36 setup.py install
    fi
    cd -
fi
fi

if [ $no_mero_rpm -eq 1 ]
then
  if [ $no_s3iamcli_build -eq 0 ]
  then
    cd auth-utils/s3iamcli/

    # Remove the s3iamcli config file if present in root directory
    rm -f /root/.sgs3iamcli/config.yaml

    if [ $no_clean_build -eq 0 ]
    then
      python36 setup.py install --force
    else
      python36 setup.py install
    fi
    # Assert to check if the certificates are installed.
    #rpm -q stx-s3-client-certs
    cd -
  fi
fi
