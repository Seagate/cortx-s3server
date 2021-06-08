#!/bin/sh -xe
cd motr
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: 1725ba73a4f2b1c8a64c8dab0f2cf8bc8e84c1e2
git checkout EOS-20955 
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
