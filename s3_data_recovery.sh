#!/bin/sh

pcs_enable_resources() {
   # enable Haproxy on both nodes
   pcs resource enable haproxy-c1 || { echo "Haproxy is not enabled" && exit 1; }
   pcs resource enable haproxy-c2 || { echo "Haproxy is not enabled" && exit 1; }

   # enable BackgroundConsumers on both nodes"
   pcs resource enable s3backcons-c1 || { echo "s3backgroundconsumer is not enabled" && exit 1; }
   pcs resource enable s3backcons-c2 || { echo "s3backgroundconsumer is not enabled" && exit 1; }

   # enable BackgroundProducer
   pcs resource enable s3backprod || { echo "s3backgroundproducer is not enabled" && exit 1; }
}

# Start Cluster
echo -e "Starting cluster\n"
pcs cluster start --all || { echo "Failed to start cluster" && exit 1; }

sleep 1m

echo -e "Cluster Started Successfully\n"

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

if [ $# -eq 0 ]
then
  if [ "$ssh" = "false" ]
  then
    s3recovery --recover || { echo "Failed to run recovery tool. Fix the failure and run the recovery tool again." && pcs_enable_resources && exit 1; }
  else
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

  echo -e "Choose one of the options to run data recovery tool with\n 1.dry_run \n 2.interactive \n 3.recover \n 4.stop \n For example: If you wish to run data recovery tool in interactive mode enter your option as interactive\n stop is for exit\n\n"  

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

