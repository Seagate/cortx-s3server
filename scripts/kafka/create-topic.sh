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

# Link for installation and configuration steps of kafka (single node and cluster) are available at https://github.com/Seagate/cortx-utils/wiki/Kafka-Server-Setup

# Disabling setting of error in script as kafka list topic command is using error condition to check topic existence. 
#set -e

#Variables
KAFKA_INSTALL_PATH=/opt
KAFKA_DIR_NAME="kafka"
BGDELETE_TOPIC_NAME="bgdelete"
consumer_count=0
hosts=""
bootstrapservers=""

#function to create topic for background delete if does not exist
create_topic() {
  echo "Creating topic..."

  cd $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME
  
  bin/kafka-topics.sh --list --bootstrap-server $HOSTNAME:9092 | grep "${BGDELETE_TOPIC_NAME}" &> /dev/null
  if [ $? -eq 1 ]; then
    echo "Topic 'bgdelete' does not exist."
	
	#create a string for bootstrap server
	create_bootstrap_servers_parameter
	
	#create topic 
	bin/kafka-topics.sh --create --topic $BGDELETE_TOPIC_NAME --bootstrap-server $bootstrapservers --replication-factor $consumer_count --partitions $consumer_count
  else
    echo "Topic 'bgdelete' already exist"
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

# Create topic for background delete 'bgdelete'
create_topic

#stop kafka and zookeeper services 
#stop_services
