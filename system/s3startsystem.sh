#!/bin/sh -x

# Restart nginx
SERVICE='nginx'

if ps ax | grep -v grep | grep $SERVICE > /dev/null
then
    echo "nginx is running"
else
    systemctl start nginx
fi

# Start the s3server
export PATH=$PATH:/opt/seagate/s3/bin

# Get local address
modprobe lnet
lctl network up &>> /dev/null
LOCAL_NID=`lctl list_nids | head -1`
LOCAL_EP=$LOCAL_NID:12345:33
HA_EP=$LOCAL_NID:12345:34:1
CONFD_EP=$LOCAL_NID:12345:33:100
PROF_OPT='<0x7000000000000001:0>'

if [[ ${1} == +([0-9]) ]]; then
  # s3server instance
  clovis_local_port=`expr 100 + $1`
  s3port=`expr 8080 + $1`
  PID_FILENAME='/var/run/s3server.'$1'.pid'
  s3server --s3pidfile $PID_FILENAME \
           --clovislocal $LOCAL_EP:${clovis_local_port} --clovisha $HA_EP \
           --clovisconfd $CONFD_EP --s3port $s3port --authhost 127.0.0.1 \
           --authport 8085 --log_dir /var/log/seagate/s3 --s3loglevel INFO
else
  # simple s3 service with default ( s3server.pid ) pid file
  s3server --clovislocal $LOCAL_EP:100 --clovisha $HA_EP \
           --clovisconfd $CONFD_EP --s3port 8081 --authhost 127.0.0.1 \
           --authport 8085 --log_dir /var/log/seagate/s3 --s3loglevel INFO
fi
