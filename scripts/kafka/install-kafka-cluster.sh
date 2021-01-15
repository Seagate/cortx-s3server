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

KAFKA_INSTALL_PATH=/opt
KAFKA_DOWNLOAD_URL="http://cortx-storage.colo.seagate.com/releases/cortx/third-party-deps/centos/centos-7.8.2003-1.0.0-3/commons/kafka/kafka_2.13-2.7.0.tgz"
KAFKA_FOLDER_NAME="kafka"
BGDELETE_TOPIC_NAME="bgdelete1"
consumer_count=0
hosts=""
bootstrapservers=""
 
install_prerequisite() {
  echo "Installing Pre-requisites"
  yum install java -y
  yum install java-devel -y
  echo "Pre-requisites installed successfully."
}

setup_kafka() {
  echo "Installing and Setting up kafka."
  cd $KAFKA_INSTALL_PATH
  curl $KAFKA_DOWNLOAD_URL -o $KAFKA_FOLDER_NAME.tgz
  mkdir $KAFKA_FOLDER_NAME
  tar -xzf $KAFKA_FOLDER_NAME.tgz -C $KAFKA_FOLDER_NAME --strip-components 1
}

start_services() {
  echo "Starting services..."
  
  cd $KAFKA_INSTALL_PATH/$KAFKA_FOLDER_NAME
  
  bin/zookeeper-server-stop.sh || true
  #start zookeeper
  nohup bin/zookeeper-server-start.sh config/zookeeper.properties &
  sleep 10s
  echo "zookeeper server started successfully."
  
  bin/kafka-server-stop.sh || true
  # start kafka server
  nohup bin/kafka-server-start.sh config/server.properties &
  sleep 10s
  echo "kafka server started successfully."
}

stop_services() {
  echo "Stopping services..."
  
  cd $KAFKA_INSTALL_PATH/$KAFKA_FOLDER_NAME
  
  # stop kafka server
  bin/kafka-server-stop.sh
  echo "kafka server stopped successfully."
  
  #stop zookeeper
  bin/zookeeper-server-stop.sh
  echo "zookeeper server started successfully."
}

configure_zookeeper() {
   echo "configure zookeeper properties"
}

configure_server() {
  echo "configure server properties" 
  
  cd $KAFKA_INSTALL_PATH/$KAFKA_FOLDER_NAME
  
  #update following properties for purge
  sed -i 's/log.retention.check.interval.ms=.*$/log.retention.check.interval.ms=1/g' config/server.properties
  sed -i '/log.retention.check.interval.ms/ a log.delete.delay.ms=1' config/server.properties
  sed -i '/log.retention.check.interval.ms/ a log.flush.offset.checkpoint.interval.ms=1' config/server.properties  
}

check_kafka_installed() {
  if [ -d "$KAFKA_INSTALL_PATH/$KAFKA_FOLDER_NAME" ]; then
    echo "Kafka is already installed"
	start_services
    exit 0
  fi
}

usage()
{
   echo ""
   echo "Usage: $0 -c consumer_count -i hosts"
   echo -e "\t-c Total number of consumers"
   echo -e "\t-i List of hosts with comma seperated e.g. host1,host2,host3"
   exit 1 # Exit script after printing help
}

while getopts c:i: flag
do
    case "${flag}" in
        c) consumer_count=${OPTARG};;
        i) hosts=${OPTARG};;
    esac
done

# Print usage in case parameters are empty
if [ $consumer_count == 0 ] || [ $hosts == "" ]
then
   echo "Some or all of the parameters are empty";
   usage
fi

echo $consumer_count
echo $hosts

IFS=',' #setting comma as delimiter  
read -a hostarr <<<"$hosts"

#Print each value of Array with the help of loop  
for (( n=0; n < ${#hostarr[*]}; n++ ))  
do  
   echo "${hostarr[n]}" 
   bootstrapservers="${bootstrapservers}${hostarr[n]}:9092,"
   echo $bootstrapservers
done 

exit 1 	

# Check kafka already installed or not 
check_kafka_installed

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
bin/kafka-topics.sh --create --topic $BGDELETE_TOPIC_NAME --bootstrap-server $bootstrapservers --replication-factor $consumer_count --partitions $consumer_count

#stop kafka and zookeeper services 
#stop_services
