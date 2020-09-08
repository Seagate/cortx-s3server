#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: bcb5342da0890611fefb604e6c203d5ab745321c

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
