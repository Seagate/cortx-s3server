#!/bin/sh
PIDSTR=`cat /var/run/s3server.$1.pid`
if [ "$PIDSTR" != "" ]; then
  kill -s TERM $PIDSTR
  if [ "$?" != "0" ]; then
    rm /var/run/s3server.$1.pid
  fi
fi
