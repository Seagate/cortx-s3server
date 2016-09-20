#!/bin/sh

if [[ ${1} == +([0-9]) ]]; then
  PIDFILE="/var/run/s3server.$1.pid"
else
  PIDFILE="/var/run/s3server.pid"
fi

PIDSTR=`cat $PIDFILE`

if [ "$PIDSTR" != "" ]; then
  kill -s TERM $PIDSTR
  if [ "$?" != "0" ]; then
    rm $PIDFILE
  fi
fi
