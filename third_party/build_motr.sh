#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: 81793c00bed8d8b60d2fe0e58a3640648e5e3eca

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
