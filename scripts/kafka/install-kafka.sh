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

set -e

#Variables
KAFKA_INSTALL_PATH=/opt
KAFKA_DOWNLOAD_URL="http://cortx-storage.colo.seagate.com/releases/cortx/third-party-deps/custom-deps/foundation/kafka-2.13_2.7.0-el7.x86_64.rpm"
KAFKA_DIR_NAME="kafka"
consumer_count=0
hosts=""
hostnumber=0
ZOOKEEPER_DIR_NAME="zookeeper"
MESSAGEBUS_CONFIG_PATH="/etc/cortx"
MESSAGEBUS_CONFIG_NAME="message_bus.conf"
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

# Function to install all pre-requisites
install_prerequisite() {
  echo "Installing Pre-requisites"
  #install confluent_kafka 1.5.0 version as this is compatible with kafka_2.13-2.7.0
  pip3 install confluent_kafka==1.5.0
  echo "Pre-requisites installed successfully."
}

#function to install kafka from rpm location
setup_kafka() {
   echo "Installing kafka."
   yum install $KAFKA_DOWNLOAD_URL -y 
   echo "Kafka installed successfully."
}

#function to start services of kafka
start_services() {
  echo "Starting services..."

  echo "Reloading systemd units."
  systemctl daemon-reload

  #start zookeeper
  systemctl start kafka-zookeeper

  # add sleep time of 10 sec before starting kafka server
  sleep 10

  is_zookeeper_started=$(systemctl is-active kafka-zookeeper)
  if [[ "$is_zookeeper_started" -eq "active" ]]; then
    echo "zookeeper server started successfully."
  else
    echo "There is a problem in starting zookeeper server!!!!\n Please check 'systemctl status kafka-zookeeper' \n"
  fi

  # start kafka server
  systemctl start kafka
  # add sleep time of 10 sec before starting kafka server
  sleep 10

  is_kafka_started=$(systemctl is-active kafka)
  if [[ "$is_kafka_started" -eq "active" ]]; then
    echo "kafka server started successfully."
  else
    echo "There is a problem in starting kafka server!!! \n Please check 'systemctl status kafka' \n"
  fi
}

#function to stop kafka services.
stop_services() {
  echo "Stopping services..."

  #stop kafka server
  systemctl stop kafka
  echo "kafka server stopped successfully."

  #stop zookeeper
  systemctl stop kafka-zookeeper
  echo "zookeeper server stopped successfully."
}

# function to Add/Edit zookeeper properties 
configure_zookeeper() {
   echo "configuring zookeeper properties"
   
   cd $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME
   
   # change data direcotory path to /opt/kafka/zookeeper
   sed -i "s+dataDir=.*$+dataDir=${KAFKA_INSTALL_PATH}/${KAFKA_DIR_NAME}/${ZOOKEEPER_DIR_NAME}+g" config/zookeeper.properties
   
   # Following properties needs to add for cluster setup only
   if [ $consumer_count -gt 1 ]; then
     sed -i '$ a tickTime=2000' config/zookeeper.properties
     sed -i '$ a initLimit=10' config/zookeeper.properties
     sed -i '$ a syncLimit=5' config/zookeeper.properties
     sed -i '$ a autopurge.snapRetainCount=3' config/zookeeper.properties
     sed -i '$ a autopurge.purgeInterval=24' config/zookeeper.properties
     
     node=1
     for (( n=0; n < ${#hostarr[*]}; n++ ))
     do
       sed -i "$ a server.${node}=${hostarr[n]}:2888:3888" config/zookeeper.properties
	     node=$((node+1))
     done
	 
	 # Set host number 
	 set_host_number

 	 # In the dataDir directory, add a file myid and add the node id ( e.g. 1 for node 1, 2 for node2, etc )
	 create_myid_file
   fi
   
   echo "zookeeper properties configured successfully"
}

# function to Add/Edit server properties 
configure_server() {
  echo "configuring server properties" 
  
  cd $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME
  
  # update log.dirs location to /opt/kafka/kafka-logs
  sed -i "s+log.dirs=.*$+log.dirs=${KAFKA_INSTALL_PATH}/${KAFKA_DIR_NAME}/kafka-logs+g" config/server.properties
  
  #update following properties for purge
  sed -i 's/log.retention.check.interval.ms=.*$/log.retention.check.interval.ms=100/g' config/server.properties
  sed -i '/log.retention.check.interval.ms/ a log.delete.delay.ms=100' config/server.properties
  sed -i '/log.retention.check.interval.ms/ a log.flush.offset.checkpoint.interval.ms=100' config/server.properties
  
  # Following properties needs to add for cluster setup only
  if [ $consumer_count -gt 1 ]; then
	sed -i "s/broker.id=.*$/broker.id=${hostnumber}/g" config/server.properties
	connect=""
    for (( n=0; n < ${#hostarr[*]}; n++ ))
    do
	  connect="${connect}${hostarr[n]}:2181,"
    done
	echo "zookeeper.connect : ${connect}"
    sed -i "s/zookeeper.connect=.*$/zookeeper.connect=${connect}/g" config/server.properties
  fi
  echo "server properties configured successfully"
}

# function to validate kafka is installed or not
is_kafka_installed() {
  if rpm -q 'kafka' ; then
    echo "Kafka is already installed. Hence stopping services and removing kafka...."
    #stop services before overwriting kafka files
    stop_services
    yum remove kafka -y
  fi
}

#function to create my id file on each host for cluster setup
create_myid_file() {
  echo "Creating myid file"
  if [ -d "${KAFKA_INSTALL_PATH}/${KAFKA_DIR_NAME}/${ZOOKEEPER_DIR_NAME}" ]; then
   echo "zookeeper directory already exists"
  else
   mkdir -p $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME/$ZOOKEEPER_DIR_NAME    
  fi
  echo $hostnumber > ${KAFKA_INSTALL_PATH}/${KAFKA_DIR_NAME}/${ZOOKEEPER_DIR_NAME}/myid
  echo "Created myid file"
}

# function to set the hostnumber for cluster
set_host_number() {

for (( n=0; n < ${#hostarr[*]}; n++ ))
do 
   if [ ${hostarr[n]} == $HOSTNAME ]; then
    hostnumber=$((n + 1))
    echo "host number is set to : ${hostnumber}"
   fi
done 
}

#function to setup message bus config file
setup_message_bus_config() {
  echo "Setup messagebus config file"
  if [ -d "$MESSAGEBUS_CONFIG_PATH" ]; then
   echo "/etc/cortx directory already exists"
  else
   mkdir -p $MESSAGEBUS_CONFIG_PATH   
   echo "/etc/cortx directory created successfully"   
  fi
  
  # copy message bus config file to /etc/cortx
  \cp ${SCRIPT_DIR}/${MESSAGEBUS_CONFIG_NAME} ${MESSAGEBUS_CONFIG_PATH}/
  echo "message_bus.config copied successfully"
}

#function to print usage/help
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

echo "Total number of consumers are: ${consumer_count}"
echo "Host(s) on which kafka will be installed and setup are: ${hosts}"

IFS=',' #setting comma as delimiter  
read -a hostarr <<<"$hosts"

# Check kafka already installed or not 
is_kafka_installed

# Install Pre-requisites 
install_prerequisite

#Setup kafka
setup_kafka

# Add/Edit configuration parameters for zookeper.properties
configure_zookeeper

# Add/Edit configuration parameters for server.properties
configure_server

#Start zookeper and kafka server 
start_services

echo "Kafka setup completed successfully."

#setup message bus config file 
setup_message_bus_config

#stop kafka and zookeeper services 
#stop_services
