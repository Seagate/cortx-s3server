#!/bin/bash
set -e

build=1
install=0
stop_bazel=0
usage() {
	echo "Usage:"
	echo "- Build s3server if run without argument"
	echo "-i : To build and install s3server binary."
	echo "-I : To only install previously build s3server binary."
	echo "-s : To stop bazel server after build."
	echo "-h : For usage."
	exit 0
}

while getopts :iIhs opt; do
	case "$opt" in
	i)
		install=1
		;;
	I)
		install=1
		build=0
		;;
	s)
		stop_bazel=1
		;;
	h)
		usage
		;;
	*)
		usage
		;;
        esac
done

if [ "$build" -eq 1 ]; then
	echo "Building s3server ..."
	bazel build //:s3server --cxxopt=-std=c++11 --define MOTR_INC=./third_party/motr/ --define MOTR_LIB=./third_party/motr/motr/.libs --define MOTR_HELPERS_LIB=./third_party/motr/helpers/.libs/ --spawn_strategy=standalone --strip=never '--local_cpu_resources=HOST_CPUS*0.7' '--local_ram_resources=HOST_RAM*0.7'
	if [ "$stop_bazel" -eq 1 ]; then
		# Just to free up resources
		echo "Stopping bazel server..."
		bazel shutdown
	fi
fi

if [ "$install" -eq 1 ]; then
	echo "Installing s3server ..."
	./dev-stops3.sh
	cp bazel-bin/s3server /opt/seagate/cortx/s3/bin/s3server
	./dev-starts3.sh
	if [ "$?" -eq 0 ]; then
		echo -e "\nS3server is started!"
	else
		echo -e "\nCould not start s3server!"
	fi
fi
