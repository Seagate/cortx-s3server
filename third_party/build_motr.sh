#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: 7ca7a3858fafaea8cc66363d2ae0ff9e8897dfd2

cd motr
# git checkout EOS-20955 
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
