#!/bin/sh -xe
# Script to build mero.
# git repo: http://es-gerrit.xyus.xyratex.com:8080/mero
# branch: master commit: 3c6e1148ff5fb18b81236700396bd7881ad61c18

cd mero
# Uncomment following line to compile mero with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
