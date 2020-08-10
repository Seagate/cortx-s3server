#!/bin/sh -xe
# Script to build motr.
# github repo: https://github.com/Seagate/cortx-motr
# branch: master commit: f0857d97ff9190de4a055781d9ec70ee52c62ef9

cd motr
# Uncomment following line to compile motr with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
