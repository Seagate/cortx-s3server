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


pcs_enable_resources() {
   # enable Haproxy on both nodes
   pcs resource enable haproxy-c1 || { echo "Haproxy is not enabled"; }
   pcs resource enable haproxy-c2 || { echo "Haproxy is not enabled"; }

   # enable BackgroundConsumers on both nodes"
   pcs resource enable s3backcons-c1 || { echo "s3backgroundconsumer is not enabled"; }
   pcs resource enable s3backcons-c2 || { echo "s3backgroundconsumer is not enabled"; }

   # enable BackgroundProducer
   pcs resource enable s3backprod || { echo "s3backgroundproducer is not enabled"; }
}

# Start Cluster
echo -e "Starting cluster\n"
pcs cluster start --all || { echo "Failed to start cluster"; }

echo -e "Sleeping for 5 minutes to make sure cluster comes up completely...\n"
sleep 5m

echo -e "Cluster Started Successfully\n"

echo -e "Cluster status..\n"
pcs cluster status

output=$(pcs status | grep s3backprod)
ssh_node=$(echo -e "$output" | tr ':' '\n' | grep "srvnode" | awk '{print $2}')
pcs_backgroundproducer_status=$(echo -e "$output" | tr ':' '\n' | grep "srvnode" | awk '{print $1}')
pcs_backgroundproducer_status=${pcs_backgroundproducer_status,,}

if [[ $pcs_backgroundproducer_status != "started" ]]
then
  echo -e "s3backgroundproducer is not running on either of the nodes. The s3backgroundproducer must be running on one of the nodes. System is not in a correct state.\n Fix the system issues and then run the s3_recovery_tool."
  exit 1
fi

#Disable Haproxy on both nodes
pcs resource disable haproxy-c1 || { echo "Haproxy is not disabled" && exit 1; }
pcs resource disable haproxy-c2 || { echo "Haproxy is not disabled" && exit 1; }

#Disable BackgroundConsumers on both nodes
pcs resource disable s3backcons-c1 || { echo "s3backgroundconsumer is not disabled" && exit 1; }
pcs resource disable s3backcons-c2 || { echo "s3backgroundconsumer is not disabled" && exit 1; }

host=$HOSTNAME
host_config="/root/.ssh/config"
while read -r line
do
  if [[ "$line" == *"$host"* && "$line" == *"srvnode"* ]]; then
     if [[ "$line" == *"srvnode-1"* ]]; then
        local_node="srvnode-1"
     else
        local_node="srvnode-2"
     fi
     break;
  fi
done < $host_config

if [[ "$local_node" == "$ssh_node" ]]
then
  ssh=false
else
  ssh=true
fi

#Disable BackgroundProducer
pcs resource disable s3backprod || { echo "s3backgroundproducer is not disabled" && exit 1; }
echo -e "s3recovery is running on node $local_node"

if [ $# -eq 0 ]
then
  if [ "$ssh" = "false" ]
  then
    s3recovery --dry_run
    s3recovery --recover || { echo "Failed to run recovery tool. Fix the failure and run the recovery tool again." && pcs_enable_resources && exit 1; }
  else
    ssh -t -T "$ssh_node" bash -c "s3recovery --dry_run"
    ssh -t -T "$ssh_node" bash -c "
    s3recovery --recover || { echo \"Failed to run recovery tool\" && exit 1; }
    " 2> /dev/null
    if [ "$?" != "0" ]; then
      echo "S3 data recovery tool has failed on remote node. Fix the failure and run s3 recovery tool again."
      pcs_enable_resources
      exit 1
    fi
  fi
else
  echo -e  "Before running any other option, it is recommended to first run with dry-run option. The dry-run option informs you about recovery items to be worked upon. \n\n"

  echo -e "Choose one of the options to run data recovery tool with\n 1.dry_run \n 2.recover \n 3.stop \n For example: If you wish to run data recovery tool in dry_run mode enter your option as dry_run\n stop is for exit\n\n"

  read -p "Option chosen is: " choice
  choice=${choice,,}
  while [ "$choice" != "stop" ]
  do
    if [ "$ssh" = false ]
    then
      s3recovery --"$choice" || { echo "Failed to run s3 recovery tool. Fix the failure and run s3recovery tool again." && pcs_enable_resources && exit 1; }
    else
      ssh -t -T "$ssh_node" bash -c "
      s3recovery --"$choice" || { echo \"Failed to run recovery tool\" && exit 1; }
      " 2> /dev/null
      if [ "$?" != "0" ]; then
        echo "S3 data recovery tool has failed on remote node. Fix the failure and run s3 recovery tool again."
        pcs_enable_resources
        exit 1
      fi    
    fi
    echo -e "Data recovery tool has been successfully run with $choice option. Enter the appropriate option either to continue or stop. \n"
    read -p "Option chosen is: " choice
  done
fi

pcs_enable_resources

