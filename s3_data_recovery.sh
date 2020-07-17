#!/bin/sh

# Start Cluster
echo -e "Starting cluster\n"
pcs cluster start --all || { echo "Failed to start cluster" && exit 1; }

sleep 1m
echo -e "Cluster Started Successfully\n"

#Disable Haproxy on both nodes
pcs resource disable haproxy-c1 || { echo "Haproxy is not disabled" && exit 1; }
pcs resource disable haproxy-c2 || { echo "Haproxy is not disabled" && exit 1; }

#Disable BackgroundConsumers on both nodes
pcs resource disable s3backcons-c1 || { echo "s3backgroundconsumer is not disabled" && exit 1; }
pcs resource disable s3backcons-c2 || { echo "s3backgroundconsumer is not disabled" && exit 1; }

is_s3backgroundproducer_running=1
systemctl is-active s3backgroundproducer 2>&1 > /dev/null || is_s3backgroundproducer_running=0
if [[ $is_s3backgroundproducer_running -eq 0 ]]; then
  host=$HOSTNAME
  host_config="/root/.ssh/config"
  echo "$host"
  while read -r line
  do
    echo "$line"
    if [[ "$line" == *"$host"* && "$line" == *"srvnode"* ]]; then
      ssh_done=true
      if [[ "$line" == *"srvnode-1"* ]]; then
        ssh srvnode-2
      else
        ssh srvnode-1
      fi
    echo -p "ssh is done\n\n\n\n\n"
    break;
    fi
  done < $host_config

if [ "$ssh_done" = true ]
then
  is_s3backgroundproducer_running=1
  systemctl is-active s3backgroundproducer 2>&1 > /dev/null || is_s3backgroundproducer_running=0
  if [[ $is_s3backgroundproducer_running -eq 0 ]]; then
    echo "s3backgroundproducer is not running on either of nodes,it must be running on one of the nodes.System is not in correct state.\nfix the system issues and then s3_recovery_tool should be run."
  exit 1
  fi
fi

#Disable BackgroundProducer
pcs resource disable s3backprod || { echo "s3backgroundproducer is not disabled" && exit 1; }
if [ $# -eq 0 ]
then
  s3recovery --recover
else
  echo -e  "Before running any other option, it is recommended to first run with dry-run option. The dry-run option informs you about recovery items to be worked upon. \n\n"

  echo -e "Choose one of the options to run data recovery tool with\n 1.dry_run \n 2.interactive \n 3.recover \n 4.stop \n For example: If you wish to run data recovery tool in interactive mode enter your option as interactive\n stop is for exit\n\n"  

  read -p "Option chosen is: " choice
  choice=${choice,,}
  while [ "$choice" != "stop" ]
  do
    s3recovery --$choice || { echo "Failed to run recovery tool" && exit 1; }
    echo -e "Data recovery tool has been successfully run with $choice option. Enter the appropriate option either to continue or stop. \n"
    read -p "Option chosen is: " choice
  done
fi
# enable Haproxy on both nodes
pcs resource enable haproxy-c1 || { echo "Haproxy is not enabled" && exit 1; }
pcs resource enable haproxy-c2 || { echo "Haproxy is not enabled" && exit 1; }

# enable BackgroundConsumers on both nodes"
pcs resource enable s3backcons-c1 || { echo "s3backgroundconsumer is not enabled" && exit 1; }
pcs resource enable s3backcons-c2 || { echo "s3backgroundconsumer is not enabled" && exit 1; }

# enable BackgroundProducer
pcs resource enable s3backprod || { echo "s3backgroundproducer is not enabled" && exit 1; }

