#!/bin/sh

# sample redhat-release contents: CentOS Linux release 7.7.1908 (Core)
os_full_version=`cat /etc/redhat-release | awk  '{ print $4 }'`
os_major_version=`echo $os_full_version | awk -F '.' '{ print $1 }'`
os_minor_version=`echo $os_full_version | awk -F '.' '{ print $2 }'`
os_build_num=`echo $os_full_version | awk -F '.' '{ print $3 }'`

# Setup the necessary yum repos as per OS.
if [ "$os_major_version" = "7" ]; then
  if [ "$os_minor_version" = "5" ]; then
    # Centos 7.5
    if [ "$os_build_num" = "1804" ]; then
      cp -f ${S3_SRC_DIR}/ansible/files/yum.repos.d/centos7.5.1804/* /etc/yum.repos.d/
    fi
  fi
  if [ "$os_minor_version" = "7" ]; then
    # Centos/RHEL 7.7
    if [ "$os_build_num" = "1908" ]; then
      cp -f ${S3_SRC_DIR}/ansible/files/yum.repos.d/centos7.7.1908/* /etc/yum.repos.d/
    fi
  fi
fi
if [ "$major_version" = "8" ]; then
  # Centos/RHEL 8
  cp -f ${S3_SRC_DIR}/ansible/files/yum.repos.d/rhel8/* /etc/yum.repos.d/
fi

# Install plugin so our repos can take priority
yum install yum-plugin-priorities -y

yum clean all
