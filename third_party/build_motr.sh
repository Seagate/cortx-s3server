#!/bin/sh -xe
cd motr
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch:  F23B_DI_PI2_Motr  commit:e5f5e1ae
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
PROCESSORS_AVAILABLE=$(cat /proc/cpuinfo | grep '^processor' | wc -l)
MAKE_OPTS=-j"$PROCESSORS_AVAILABLE" ./scripts/m0 rebuild
cd ..
