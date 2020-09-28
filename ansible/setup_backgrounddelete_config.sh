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
# As of now, s3backgrounddelete uses rabbitmq server to communicate
# message between producer and consumer. So we need to configure Rabbitmq
# on the system. Refer: <cortx-s3server>/s3backgrounddelete/rabbitmq_setup.md

# Install rabbit-mq.
# As of now rabbit-mq is being installed as part of motr dependency installation.
# curl -s https://packagecloud.io/install/repositories/rabbitmq/rabbitmq-server/script.rpm.sh | sudo bash


centos_release=`cat /etc/centos-release | awk '/CentOS/ {print}'`
redhat_release=`cat /etc/redhat-release | awk '/Red Hat/ {print}'`

if [ ! -z "$centos_release" ]
then
    rpm -q python36-pika || yum -y install python36-pika    
elif [ ! -z "$redhat_release" ]
then
    rpm -q python36-pika || yum -y localinstall https://dl.fedoraproject.org/pub/epel/8/Everything/x86_64/Packages/p/python3-pika
fi

# Start rabbitmq
systemctl start rabbitmq-server

# Automatically start RabbitMQ at boot time
systemctl enable rabbitmq-server

# Create a user (In this case user is cortx-s3 with password as password)
rabbitmquser=`rabbitmqctl list_users | awk '/cortx-s3/ {print}'`

if [ -z "$rabbitmquser" ]
then
    rabbitmqctl add_user cortx-s3 password
fi

# Setup the cortx-s3 user as an administrator.
rabbitmqctl set_user_tags cortx-s3 administrator

# Set permission
rabbitmqctl set_permissions -p / cortx-s3 "." "." ".*"

# Setup queue mirroring
rabbitmqctl set_policy ha-all ".*" '{"ha-mode":"all"}'

