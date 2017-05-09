#!/bin/sh
# Script to stop S3 server in dev environment.
# The script will stop all running instances of S3 server.

MAX_S3_INSTANCES_NUM=20

instance=1
while [[ $instance -le $MAX_S3_INSTANCES_NUM ]]
do
  s3port=$((8080 + $instance))
  pidfile="/var/run/s3server.$s3port.pid"
  if [[ -r $pidfile ]]; then
    pidstr=$(cat $pidfile)
    if [ "$pidstr" != "" ]; then
      kill -s TERM $pidstr
    fi # $pidstr
  fi # $pidfile

  ((instance++))
done # while
echo "Waiting for S3 to shutdown..."
sleep 10

instance=1
while [[ $instance -le $MAX_S3_INSTANCES_NUM ]]
do
  statuss3=$(./iss3up.sh $instance)
  if [ "$statuss3" != "" ]; then
    s3port=$((8080 + $instance))
    pidfile="/var/run/s3server.$s3port.pid"
    if [[ -r $pidfile ]]; then
      pidstr=$(cat $pidfile)
      kill -9 $pidstr
    fi
  fi

  if [[ -e $pidfile ]]; then
    rm -f $pidfile
  fi

  ((instance++))
done
