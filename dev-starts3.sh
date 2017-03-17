#!/bin/sh -e
# Script to start S3 server in dev environment.
#   Usage: sudo ./dev-starts3.sh [<Number of S3 sever instances>]
#               Optional argument is:
#                   Number of S3 server instances to start.
#                   Max number of instances allowed = 20
#                   Default number of instances = 1

MAX_S3_INSTANCES_NUM=20

if [ $# -gt 1 ]
then
  echo "Invalid number of arguments passed to the script"
  echo "Usage: sudo ./dev-starts3.sh [<Number of S3 sever instances>]"
  exit 1
fi

num_instances=1

if [ $# -eq 1 ]
then
  if [[ $1 =~ ^[0-9]+$ ]] && [ $1 -le $MAX_S3_INSTANCES_NUM ]
  then
    num_instances=$1
  else
    echo "Invalid argument [$1], needs to be an integer <= $MAX_S3_INSTANCES_NUM"
    exit 1
  fi
fi

set -x

export LD_LIBRARY_PATH="$(pwd)/third_party/mero/mero/.libs/:"\
"$(pwd)/third_party/mero/extra-libs/gf-complete/src/.libs/"

# Restart nginx
SERVICE='nginx'

if ps ax | grep -v grep | grep $SERVICE > /dev/null
then
    echo "nginx is running"
else
    systemctl start nginx
fi

# Get local address
modprobe lnet
lctl network up &>> /dev/null
local_nid=`lctl list_nids | head -1`
local_ep=$local_nid:12345:33
ha_ep=$local_nid:12345:34:1

# Run m0dixinit
set +e
./third_party/mero/dix/utils/m0dixinit -l $local_nid:12345:34:100 -H $local_nid:12345:34:1 \
                 -p '<0x7000000000000001:0>' -I 'v|1:20' -d 'v|1:20' -a check 2> /dev/null \
                 | grep -E 'Metadata exists: false' > /dev/null
rc=$?
set -e
if [ $rc -eq 0 ]
then
./third_party/mero/dix/utils/m0dixinit -l $local_nid:12345:34:100 -H $local_nid:12345:34:1 \
                 -p '<0x7000000000000001:0>' -I 'v|1:20' -d 'v|1:20' -a create
fi

# Start the s3server
export PATH=$PATH:/opt/seagate/s3/bin
counter=1
while [[ $counter -le $num_instances ]]
do
  clovis_local_port=`expr 100 + $counter`
  s3port=`expr 8080 + $counter`
  pid_filename='/var/run/s3server.'$counter'.pid'
  s3server --s3pidfile $pid_filename \
           --clovislocal $local_ep:${clovis_local_port} --clovisha $ha_ep \
           --s3port $s3port
  ((counter++))
done
