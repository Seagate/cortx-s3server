#!/bin/bash

#####################################################
#
# This file can be placed in system executable path,
# for easy access to the script.
#
# cp s3log.sh /usr/local/sbin/s3log
#
#####################################################

info=0
debug=0
warn=0
error=0
tail=0
vim=0
auth=0
bd=0
bd_schedular=0

usage() {
		echo "Usage:"
		echo "-i : For info logs."
		echo "-d : For debug logs."
		echo "-w : For warning logs."
		echo "-e : For error logs."
		echo "## Note: Only one of the above options can be used."
		echo "-a : To see auth server logs. Can not be used along with -b."
		echo "-b : To see background processor logs. Can not be used along with -a."
		echo "-s : To see background schedular logs. To be used along with -b."
		echo "-t : To tail log file. Can not be used with -v option."
		echo "-v : To open log in vim editor. Can not be used with -t option."
		echo "-h : For usage."
		exit 1
}

if [ "$#" -eq 0 ]; then
	usage
fi
while getopts :idwetvhabs opt; do
	case "$opt" in
	i)
		info=1
		if [ "$debug" -eq 1 ] || [ "$error" -eq 1 ] || [ "$warn" -eq 1 ]; then
			usage
		fi
		;;
	d)
		debug=1
		if [ "$info" -eq 1 ] || [ "$error" -eq 1 ] || [ $warn -eq 1 ]; then
			usage
		fi
		;;
	w)
		warn=1
		if [ "$info" -eq 1 ] || [ "$error" -eq 1 ] || [ "$debug" -eq 1 ]; then
				usage
		fi
		;;
	e)
		error=1
		if [ "$debug" -eq 1 ] || [ "$info" -eq 1 ] || [ "$warn" -eq 1 ]; then
			usage
		fi
		;;
	t)
		tail=1
		if [ "$vim" -eq 1 ]; then
			usage
		fi
		;;
	v)
		vim=1
		if [ "$tail" -eq 1 ]; then
			usage
		fi
		;;
	a)
		auth=1
		if [ "$bd" -eq 1 ]; then
			usage
		fi
		;;
	b)
		bd=1
		if [ "$auth" -eq 1 ]; then
			usage
		fi
		;;
	s)
		bd_schedular=1
		;;
	h)
		usage
		;;
	*)
		usage
		;;
	esac
done

if [ -z $info ] && [ -z $debug ] && [ -z $error ] && [ -z $warn ] && [ ! -z $auth ] && [ ! -z $bd ]; then
	usage
fi

if [ -z $tail ] && [ -z $vim ]; then
	usage
fi

filename=
if [ $info -eq 1 ]; then
	filename=/var/log/cortx/s3/s3server.INFO
fi

if [ $debug -eq 1 ]; then
	filename=/var/log/cortx/s3/s3server.INFO
fi

if [ $warn -eq 1 ]; then
	filename=/var/log/cortx/s3/s3server.WARNING
fi

if [ $error -eq 1 ]; then
	filename=/var/log/cortx/s3/s3server.ERROR
fi

if [ $auth -eq 1 ]; then
	filename=/var/log/cortx/auth/server/app.log
fi

if [ $bd -eq 1 ]; then
	filename=/var/log/cortx/s3/s3backgrounddelete/object_recovery_processor.log
	if [  $bd_schedular -eq 1 ]; then
		filename=/var/log/cortx/s3/s3backgrounddelete/object_recovery_scheduler.log
	fi
fi

if [ $tail -eq 1 ]; then
	tail -f $filename
	exit 0
fi

if [ $vim -eq 1 ]; then
	vim $filename
	exit 0
fi
