#!/bin/bash

s3ut=0
s3utdeathtests=0
s3mempoolut=0
s3mempoolmgrut=0
stop_bazel=0

usage() {
	echo "Usage:"
	echo "--all : To build all the UTs run without argument."
	echo "--ut : To build only s3ut."
	echo "--death : To build s3utdeathtests."
	echo "--mempool : To build s3mempoolut."
	echo "--mempoolmgr : To build s3mempoolmgrut."
	echo "-s : Stop bazel server after build."
	exit 0
}

if [ "$#" -eq 0 ]; then
	usage
else
	while [ "$1" != "" ]; do
	case "$1" in
	--ut)
		s3ut=1
		;;
	--death)
		s3utdeathtests=1
		;;
	--mempool)
		s3mempoolut=1
		;;
	--mempoolmgr)
		s3mempoolmgrut=1
		;;
	--all)
		s3ut=1
		s3utdeathtests=1
		s3mempoolut=1
		s3mempoolmgrut=1
		;;
	-s)
		stop_bazel=1
		;;
	--help | -h)
		usage
		;;
	*)
		usage
		;;
	esac
	shift
	done
fi

bazel_cpu_limit=0.6
bazel_ram_limit=0.6

if [ "$s3ut" -eq 1 ]; then
	echo "Building s3ut..."
	bazel build //:s3ut --cxxopt=-std=c++11 --define MOTR_INC=./third_party/motr/ --define MOTR_LIB=./third_party/motr/motr/.libs --define MOTR_HELPERS_LIB=./third_party/motr/helpers/.libs/ --spawn_strategy=standalone --strip=never "--local_cpu_resources=HOST_CPUS*$bazel_cpu_limit" "--local_ram_resources=HOST_RAM*$bazel_ram_limit" --verbose_failures

fi

if [ "$s3utdeathtests" -eq 1 ]; then
	echo "Building s3utdeathtests..."
	bazel build //:s3utdeathtests --cxxopt=-std=c++11 --define MOTR_INC=./third_party/motr/ --define MOTR_LIB=./third_party/motr/motr/.libs --define MOTR_HELPERS_LIB=./third_party/motr/helpers/.libs/ --spawn_strategy=standalone --strip=never "--local_cpu_resources=HOST_CPUS*$bazel_cpu_limit" "--local_ram_resources=HOST_RAM*$bazel_ram_limit"

fi

if [ "$s3mempoolut" -eq 1 ]; then
	echo "Building s3mempoolut..."
	bazel build //:s3mempoolut --cxxopt=-std=c++11 --spawn_strategy=standalone --strip=never "--local_cpu_resources=HOST_CPUS*$bazel_cpu_limit" "--local_ram_resources=HOST_RAM*$bazel_ram_limit"

fi

if [ "$s3mempoolmgrut" -eq 1 ]; then
	echo "Building s3mempoolmgrut..."
	bazel build //:s3mempoolmgrut --cxxopt=-std=c++11 --define MOTR_INC=./third_party/motr/ --define MOTR_LIB=./third_party/motr/motr/.libs --define MOTR_HELPERS_LIB=./third_party/motr/helpers/.libs/ --spawn_strategy=standalone --strip=never "--local_cpu_resources=HOST_CPUS*$bazel_cpu_limit" "--local_ram_resources=HOST_RAM*$bazel_ram_limit"

fi

if [  "$stop_bazel" -eq 1 ]; then
	# Just to free up resources
	echo "Stopping bazel server..."
	bazel shutdown
fi
