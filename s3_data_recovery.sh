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

#Disable BackgroundProducer
pcs resource disable s3backprod || { echo "s3backgroundproducer is not disabled" && exit 1; }

echo -e  "Before running any other option, it is recommended to first run with dry-run option. The dry-run option informs you about recovery items to be worked upon. \n\n"

echo -e "Choose one of the options to run data recovery tool with\n 1.dry_run \n 2.interactive \n 3.recover \n 4.stop \n For example: If you wish to run data recovery tool in interactive mode enter your option as interactive\n stop is for exit\n\n"

read -p "Option chosen is: " choice
while [ "$choice" != "stop" ]
do
    s3recovery --$choice || { echo "Failed to run recovery tool" && exit 1; }
    echo -e "Data recovery tool has been successfully run with $choice option. Enter the appropriate option either to continue or stop. \n"
    read -p "Option chosen is: " choice
done

# enable Haproxy on both nodes
pcs resource enable haproxy-c1 || { echo "Haproxy is not disabled" && exit 1; }
pcs resource enable haproxy-c2 || { echo "Haproxy is not disabled" && exit 1; }

# enable BackgroundConsumers on both nodes"
pcs resource enable s3backcons-c1 || { echo "s3backgroundconsumer is not disabled" && exit 1; }
pcs resource enable s3backcons-c2 || { echo "s3backgroundconsumer is not disabled" && exit 1; }

# enable BackgroundProducer
pcs resource enable s3backprod || { echo "s3backgroundproducer is not disabled" && exit 1; }

