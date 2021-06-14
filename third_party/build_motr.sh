#!/bin/sh -xe
cd motr
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: 70701eea1dd1e20fd7c6bdc5bdd1203795d09958

cd motr
git checkout EOS-20955 
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
