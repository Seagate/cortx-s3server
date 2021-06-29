#!/bin/sh
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#

set -e

# Reference <cortx-s3server>/s3backgrounddelete/readme.md file
#
# s3backgrounddelete uses cortx message bus to communicate
# messages between producer and consumer.

centos_release=`cat /etc/centos-release | awk '/CentOS/ {print}'`
redhat_release=`cat /etc/redhat-release | awk '/Red Hat/ {print}'`

if [ ! -z "$centos_release" ]
then
    rpm -q python36-pika || yum -y install python36-pika    
elif [ ! -z "$redhat_release" ]
then
    rpm -q python36-pika || yum -y localinstall https://dl.fedoraproject.org/pub/epel/8/Everything/x86_64/Packages/p/python3-pika
fi
