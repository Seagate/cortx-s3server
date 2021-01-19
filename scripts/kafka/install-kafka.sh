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
KAFKA_DOWNLOAD_URL="http://cortx-storage.colo.seagate.com/releases/cortx/third-party-deps/centos/centos-7.8.2003-1.0.0-3/commons/kafka/kafka_2.13-2.7.0.tgz"
KAFKA_DIR_NAME="kafka"
BGDELETE_TOPIC_NAME="bgdelete"
consumer_count=0
hosts=""
bootstrapservers=""
hostnumber=0
ZOOKEEPER_DIR_NAME="zookeeper"

# Function to install all pre-requisites
install_prerequisite() {
  echo "Installing Pre-requisites"
  yum install java -y
  yum install java-devel -y
  echo "Pre-requisites installed successfully."
}

# function to download and setup kafka
setup_kafka() {
  echo "Installing and Setting up kafka."
  cd $KAFKA_INSTALL_PATH
  curl $KAFKA_DOWNLOAD_URL -o $KAFKA_DIR_NAME.tgz
  if [ -d "$KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME" ]; then
    echo "kafka directory is already exist"
  else
    mkdir -p $KAFKA_DIR_NAME    
  fi
  tar -xzf $KAFKA_DIR_NAME.tgz -C $KAFKA_DIR_NAME --strip-components 1
}

#function to start services of kafka
start_services() {
  echo "Starting services..."
  
  cd $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME
  
  # stop kafka and zookeper service if they are runnig
  stop_services
    
  #start zookeeper
  bin/zookeeper-server-start.sh -daemon config/zookeeper.properties
  sleep 10s
  echo "zookeeper server started successfully."
  netstat -tulnp | grep 2181
  
  # start kafka server
  bin/kafka-server-start.sh -daemon config/server.properties
  sleep 10s
  echo "kafka server started successfully."
  netstat -tulnp | grep 9092
}

#function to stop kafka services.
stop_services() {
  echo "Stopping services..."
  
  cd $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME
  
  # stop kafka server
  bin/kafka-server-stop.sh -daemon config/server.properties || true
  echo "kafka server stopped successfully."
  netstat -tulnp | grep 9092
  
  #stop zookeeper
  bin/zookeeper-server-stop.sh -daemon config/zookeeper.properties || true
  echo "zookeeper server stopped successfully."
  netstat -tulnp | grep 2181
}

# function to Add/Edit zookeeper properties 
configure_zookeeper() {
   echo "configure zookeeper properties"
   
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
     for i in $(echo $hosts | sed "s/,/ /g")
     do
       sed -i "$ a server.${node}=${i}:2888:3888" config/zookeeper.properties
	   node=$node+1
     done
	 
	 # Set host number 
	 set_host_number

 	 # In the dataDir directory, add a file myid and add the node id ( e.g. 1 for node 1, 2 for node2, etc )
	 create_myid_file
   fi
   
}

# function to Add/Edit server properties 
configure_server() {
  echo "configure server properties" 
  
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
    for host in $(echo $hosts | sed "s/,/ /g")
    do
	  connect="${connect}${host}:2181,"
    done
	echo "zookeeper.connect : ${connect}"
    sed -i "s/zookeeper.connect=.*$/zookeeper.connect=${connect}/g" config/server.properties
  fi

}

# function to validate kafka is installed or not
is_kafka_installed() {
  if [ -d "$KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME" ]; then
    echo "Kafka is already installed"
	#stop services before overwriting kafka files
	stop_services
  fi
}

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

#function to create my id file on each host for cluster setup
create_myid_file() {
  echo "Creating myid file"
  if [ -d "${KAFKA_INSTALL_PATH}/${KAFKA_DIR_NAME}/${ZOOKEEPER_DIR_NAME}" ]; then
   echo "zookeeper directory is already exist"
  else
   mkdir -p $KAFKA_INSTALL_PATH/$KAFKA_DIR_NAME/$ZOOKEEPER_DIR_NAME    
  fi
  echo $hostnumber > ${KAFKA_INSTALL_PATH}/${KAFKA_DIR_NAME}/${ZOOKEEPER_DIR_NAME}/myid
  echo "Created myid file"
}

# function to set the hostnumber for cluster
set_host_number() {

IFS=',' #setting comma as delimiter  
read -a hostarr <<<"$hosts"

for (( n=0; n < ${#hostarr[*]}; n++ ))  
do 
   if [ ${hostarr[n]} == $HOSTNAME ]; then
    hostnumber=$((n + 1))
    echo "host number is set to : ${hostnumber}"
   fi
done 
}

#function to create a bootstrap server parameter for creating topic
create_bootstrap_servers_parameter() {

for i in $(echo $hosts | sed "s/,/ /g")
do
    bootstrapservers="${bootstrapservers}${i}:9092,"
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

echo "Total number of consumers are: ${consumer_count}"
echo "Host(s) on which kafka will be installed and setup are: ${hosts}"

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

# Create topic for background delete 'bgdelete'
create_topic

#stop kafka and zookeeper services 
#stop_services
