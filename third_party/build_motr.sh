#!/bin/sh -xe
cd motr
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: 2ca587c61fdbd2b2b71c79ab0ebd8761e496f3ab

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
