#!/bin/sh -xe
# Script to build mero.
# git repo: http://es-gerrit.xyus.xyratex.com:8080/mero
# branch: master commit: 4633703d0782bf618a1090240c418212d5968151

cd mero
# Uncomment following line to compile mero with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
