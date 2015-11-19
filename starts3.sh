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
CONFD_EP=$LOCAL_NID:12345:33:100
PROF_OPT='<0x7000000000000001:0>'

s3server -l $LOCAL_EP -c $CONFD_EP -p 8081 -s 127.0.0.1 -d 8085
