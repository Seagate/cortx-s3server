#!/bin/sh

set -e

# When run with no args, builds all packages.  Can specify package name as 1st
# arg to build just that package.

pkg_list="[ wheel, jmespath, xmltodict, botocore, s3transfer, boto3 ]"
USAGE="USAGE: bash $(basename "$0") [--all | --python34 | --python36 ]
                                    [--pkg <module to build>]
    Builds python modules:
    $pkg_list.
where:
--all         default. Build for both Python 3.4 & 3.6
--python34    Build for python 3.4
--python36    Build for python 3.6
--pkg         Package to build. When not specified build all packages."

build_for_all_py_vers=0
build_for_py_34=0
build_for_py_36=0
pkg=""

if [ $# -eq 0 ]
then
  build_for_all_py_vers=1
else
  while [ "$1" != "" ]; do
    case "$1" in
      --all ) build_for_all_py_vers=1;
          ;;
      --python34 ) build_for_py_34=1;
          ;;
      --python36 ) build_for_py_36=1;
          ;;
      --pkg )
          if [ "$2" = "wheel" ] || \
             [ "$2" = "jmespath" ] || \
             [ "$2" = "xmltodict" ] || \
             [ "$2" = "botocore" ] || \
             [ "$2" = "s3transfer" ] || \
             [ "$2" = "boto3" ]
          then
            pkg="$2";
          else
            echo "ERROR: Invalid package specified."
            echo "---------------------------------"
            echo ""
            echo "$USAGE"
            exit 1
          fi
          shift;
          ;;
      --help | -h )
          echo "$USAGE"
          exit 1
          ;;
      * )
          echo "$USAGE"
          exit 1
          ;;
    esac
    shift
  done
fi

need_pkg() {
  test -z "$pkg" -o x"$pkg" = x"$1"
}

install_python36_deps() {
  yum install -y python36 python36-devel
  # Few packages needed for python36 are not available in our epel mirror.
  # We download directly till we get latest epel.
  yum info python36-six > /dev/null || \
    yum install -y https://mirror.umd.edu/fedora/epel/7/x86_64/Packages/p/python36-six-1.11.0-3.el7.noarch.rpm
  yum info python36-docutils > /dev/null || \
    yum install -y https://mirror.umd.edu/fedora/epel/7/x86_64/Packages/p/python36-docutils-0.14-1.el7.noarch.rpm
  yum info python36-dateutil > /dev/null || \
    yum install -y https://mirror.umd.edu/fedora/epel/7/x86_64/Packages/p/python36-dateutil-2.4.2-5.el7.noarch.rpm
  yum info python36-pbr > /dev/null || \
    yum install -y https://mirror.umd.edu/fedora/epel/7/x86_64/Packages/p/python36-pbr-4.2.0-3.el7.noarch.rpm
  yum info python36-nose > /dev/null || \
    yum install -y https://mirror.umd.edu/fedora/epel/7/x86_64/Packages/p/python36-nose-1.3.7-4.el7.noarch.rpm
  yum info python36-mock > /dev/null || \
    yum install -y https://mirror.umd.edu/fedora/epel/7/x86_64/Packages/p/python36-mock-2.0.0-2.el7.noarch.rpm
}

install_python34_deps() {
  yum install -y python34 python34-devel
}

# Prepare custom options list for rpmbuild & yum-builddep:
rpmbuild_opts=()
yumdep_opts=()

if [[ build_for_all_py_vers -eq 1 ]]; then
  echo "Building packages for python 3.4 & python 3.6 ..."
  install_python34_deps
  install_python36_deps
  rpmbuild_opts=(--define 's3_with_python34 1' --define 's3_with_python36 1')
  yumdep_opts=(--define 's3_with_python34 1' --define 's3_with_python36 1')
fi
if [[ build_for_py_34 -eq 1 ]]; then
  echo "Building packages for python 3.4 ..."
  install_python34_deps
  rpmbuild_opts=(--define 's3_with_python34 1')
  yumdep_opts=(--define 's3_with_python34 1')
fi
if [[ build_for_py_36 -eq 1 ]]; then
  echo "Building packages for python 3.6 ..."
  install_python36_deps
  rpmbuild_opts=("${rpmbuild_opts[@]}" --define 's3_with_python36 1')
  yumdep_opts=("${yumdep_opts[@]}" --define 's3_with_python36 1')
fi
if [[ $pkg = "" ]]; then
  echo "Building all packages $pkg_list..."
else
  echo "Building package $pkg..."
fi

echo "rpmbuild_opts = ${rpmbuild_opts[*]}"
echo "yumdep_opts = ${yumdep_opts[*]}"

WHEEL_VERSION=0.24.0
JMESPATH_VERSION=0.9.0
XMLTODICT_VERSION=0.9.0
S3TRANSFER_VERSION=0.1.10
BOTOCORE_VERSION=1.6.0
BOTO3_VERSION=1.4.6

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

cd ~/rpmbuild/SOURCES/
rm -rf wheel*
rm -rf jmespath*
rm -rf xmltodict*
rm -rf s3transfer*
rm -rf botocore*
rm -rf boto3*


#wheel
if need_pkg wheel; then
  wget https://bitbucket.org/dholth/wheel/raw/099352e/wheel/test/test-1.0-py2.py3-none-win32.whl
  wget https://bitbucket.org/dholth/wheel/raw/099352e/wheel/test/pydist-schema.json
  wget https://pypi.python.org/packages/source/w/wheel/wheel-${WHEEL_VERSION}.tar.gz
fi

#jmespath
if need_pkg jmespath; then
  wget https://pypi.python.org/packages/source/j/jmespath/jmespath-${JMESPATH_VERSION}.tar.gz
fi

#xmltodict
if need_pkg xmltodict; then
  wget https://pypi.python.org/packages/source/x/xmltodict/xmltodict-${XMLTODICT_VERSION}.tar.gz
fi

#botocore
if need_pkg botocore; then
  wget https://pypi.io/packages/source/b/botocore/botocore-${BOTOCORE_VERSION}.tar.gz
fi

#s3transfer
if need_pkg s3transfer; then
  wget https://pypi.io/packages/source/s/s3transfer/s3transfer-${S3TRANSFER_VERSION}.tar.gz
fi

#boto3
if need_pkg boto3; then
  wget https://pypi.io/packages/source/b/boto3/boto3-${BOTO3_VERSION}.tar.gz
fi

#copy patches
cp ${BASEDIR}/botocore/botocore-1.5.3-fix_dateutil_version.patch ~/rpmbuild/SOURCES/

cd -

# Install python macros
yum install python3-rpm-macros -y

need_pkg wheel      && yum-builddep -y ${BASEDIR}/wheel/python-wheel.spec "${yumdep_opts[@]}" ; \
                       rpmbuild -ba ${BASEDIR}/wheel/python-wheel.spec "${rpmbuild_opts[@]}"

need_pkg jmespath   && yum-builddep -y ${BASEDIR}/jmespath/python-jmespath.spec "${yumdep_opts[@]}" ; \
                       rpmbuild -ba ${BASEDIR}/jmespath/python-jmespath.spec "${rpmbuild_opts[@]}"

need_pkg xmltodict  && yum-builddep -y ${BASEDIR}/xmltodict/python-xmltodict.spec "${yumdep_opts[@]}" ; \
                        rpmbuild -ba ${BASEDIR}/xmltodict/python-xmltodict.spec "${rpmbuild_opts[@]}"

need_pkg botocore   && yum-builddep -y ${BASEDIR}/botocore/python-botocore.spec "${yumdep_opts[@]}" ; \
                       rpmbuild -ba ${BASEDIR}/botocore/python-botocore.spec "${rpmbuild_opts[@]}"

# These deps are required for s3transfer
yum localinstall ~/rpmbuild/RPMS/noarch/python3* -y

need_pkg s3transfer && yum-builddep -y ${BASEDIR}/s3transfer/python-s3transfer.spec "${yumdep_opts[@]}" ; \
                       rpmbuild -ba ${BASEDIR}/s3transfer/python-s3transfer.spec "${rpmbuild_opts[@]}"

# s3transfer dep required for boto3
yum localinstall ~/rpmbuild/RPMS/noarch/python3* -y

need_pkg boto3      && yum-builddep -y ${BASEDIR}/boto3/python-boto3.spec "${yumdep_opts[@]}" ;
                       rpmbuild -ba ${BASEDIR}/boto3/python-boto3.spec "${rpmbuild_opts[@]}"
