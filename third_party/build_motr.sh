#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# motr commit: 3a429ad5 (main branch)

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
