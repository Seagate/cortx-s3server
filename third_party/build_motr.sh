#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: 44bbdb3e9f94ac1339accfbdbb7c55bf961dbae2

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
