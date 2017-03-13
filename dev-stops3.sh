#!/bin/sh
# Script to stop S3 server in dev environment.
# The script will stop all running instances of S3 server.

MAX_S3_INSTANCES_NUM=20

counter=1
while [[ $counter -le $MAX_S3_INSTANCES_NUM ]]
do
  pidfile="/var/run/s3server.$counter.pid"
  if [[ -r $pidfile ]]; then
    pidstr=`cat $pidfile`
    if [ "$pidstr" != "" ]; then
      kill -s TERM $pidstr
      if [ "$?" != "0" ]; then
        rm $pidfile
      fi
    fi # $pidstr
  fi # $pidfile

  ((counter++))
done # while

sleep 10
