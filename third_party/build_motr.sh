#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: 2a585e486eadaf659b246df921f2d5a96fca6f37

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
