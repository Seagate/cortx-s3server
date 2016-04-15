#!/bin/sh

for i in `ls /var/run/s3server-*.pid 2> /dev/null`
do
PIDSTR=`cat $i`
read -p "Shutdown s3 server daemon with pid $PIDSTR (y/n)? " answer
case ${answer:0:1} in
  y|Y )
      kill -s TERM $PIDSTR
      if [ "$?" != "0" ]; then
        rm $i
      fi
  ;;
esac
done
