#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: f67c69f0da129c70b761915f45156ac6af355928

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
