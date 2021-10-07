#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# motr commit: 345023e777881895962e5af3d392722879423cbf (main branch)

cd motr

# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
