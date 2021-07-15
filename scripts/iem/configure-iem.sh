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

# Disabling setting of error in script as kafka list topic command is using error condition to check topic existence. 
#set -e

#Variables
KAFKA_INSTALL_PATH=/opt
KAFKA_DIR_NAME="kafka"
IEM_TOPIC_NAME="IEM"
consumer_count=0
hosts=""
bootstrapservers=""
CLUSTER_CONFIG_PATH="/etc/cortx"
CLUSTER_CONFIG_NAME="cluster.conf"
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

#function to create topic for background delete if does not exist
create_topic() {
  echo "Creating topic..."

  cd $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME

  bin/kafka-topics.sh --list --bootstrap-server $HOSTNAME:9092 | grep "${IEM_TOPIC_NAME}" &> /dev/null
  if [ $? -eq 1 ]; then
    echo "Topic 'IEM' does not exist."
	
	#create a string for bootstrap server
	create_bootstrap_servers_parameter
	
	#create topic 
	bin/kafka-topics.sh --create --topic $IEM_TOPIC_NAME --bootstrap-server $bootstrapservers --replication-factor $consumer_count --partitions $consumer_count
  else
    echo "Topic 'IEM' already exist"
  fi
}

#function to create a bootstrap server parameter for creating topic
create_bootstrap_servers_parameter() {

IFS=',' #setting comma as delimiter  
read -a hostarr <<<"$hosts"

for (( n=0; n < ${#hostarr[*]}; n++ ))
do
    bootstrapservers="${bootstrapservers}${hostarr[n]}:9092,"
done

echo "Bootstrap servers string: ${bootstrapservers}"
}

#function to setup cluster config file
setup_cluster_config() {
  echo "Setup cluster config file"
  if [ -d "$CLUSTER_CONFIG_PATH" ]; then
   echo "/etc/cortx directory already exists"
  else
   mkdir -p $CLUSTER_CONFIG_PATH
   echo "/etc/cortx directory created successfully"
  fi
  
  # copy message bus config file to /etc/cortx
  \cp ${SCRIPT_DIR}/${CLUSTER_CONFIG_NAME} ${CLUSTER_CONFIG_PATH}/
  echo "cluster.conf copied successfully"
}

#function to start message bus server
start_server() {
  echo "Starting Message bus server"

  # start message bus server
  systemctl start cortx_message_bus

  sleep 10

  is_cortx_message_bus_started=$(systemctl is-active cortx_message_bus)
  if [[ "$is_cortx_message_bus_started" -eq "active" ]]; then
    echo "message bus server started successfully. !!!!\n Please check 'systemctl status cortx_message_bus' \n"
  else
    echo "There is a problem in starting message bus server."
  fi
}

usage()
{
   echo ""
   echo "Usage: $0 -c consumer_count -i hosts"
   echo -e "\t-c Total number of consumers"
   echo -e "\t-i List of hosts with comma seperated e.g. host1,host2,host3"
   echo -e "\t-h Help" 1>&2;
   exit 1 # Exit script after printing help
}

###############################
### Main script starts here ###
###############################

# parse the command line arguments
while getopts c:i:h: flag
do
    case "${flag}" in
        c) consumer_count=${OPTARG};;
        i) hosts=${OPTARG};;
        h) usage ;;
    esac
done

# Print usage in case parameters are empty
if [ $consumer_count == 0 ] || [ $hosts == "" ]
then
   echo "Some or all of the parameters are empty";
   usage
fi

echo "Total number of consumers to create topic: ${consumer_count}"
echo "Total number of host(s) on which topic will be created: ${hosts}"

# Create topic 'IEM' for Interesting Event Message
create_topic

# copy cluster.conf to /etc/cortx/
setup_cluster_config

# Start Message bus Rest Server
start_server
