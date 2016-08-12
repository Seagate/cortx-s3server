#!/bin/sh -x

# Restart nginx
systemctl restart nginx

# Start the s3server
export PATH=$PATH:/opt/seagate/s3/bin

# Get local address
modprobe lnet
lctl network up &>> /dev/null
LOCAL_NID=`lctl list_nids | head -1`
LOCAL_EP=$LOCAL_NID:12345:33:100
HA_EP=$LOCAL_NID:12345:34:1
CONFD_EP=$LOCAL_NID:12345:33:100
PROF_OPT='<0x7000000000000001:0>'
s3server --clovislocal $LOCAL_EP --clovisha $HA_EP --clovisconfd $CONFD_EP --s3port 8081 --authhost 127.0.0.1 --authport 8085 --log_dir /var/log/seagate/s3 --s3loglevel INFO
