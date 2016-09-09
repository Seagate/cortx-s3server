#!/bin/sh
for s3unit in `systemctl list-units | grep s3server@  | grep "loaded active running"  | awk '{print $1}'`
do
  systemctl stop $s3unit
done

out=`systemctl list-units | grep s3server | grep "loaded active running"  | awk '{print $1}'`
if [ "$out" != "" ]; then
  systemctl stop s3server
fi

PIDSTR=`cat /var/run/s3server.pid 2> /dev/null`
if [ "$PIDSTR" != "" ]; then
  read -p "Shutdown s3 server daemon with pid $PIDSTR (y/n)? " answer
  case ${answer:0:1} in
    y|Y )
      kill -s TERM $PIDSTR
      if [ "$?" != "0" ]; then
        rm /var/run/s3server.pid
      fi
  ;;
esac
fi
