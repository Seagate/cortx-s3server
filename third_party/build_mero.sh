#!/bin/sh -xe
# Script to build mero.
# git repo: http://es-gerrit.xyus.xyratex.com:8080/mero
# branch: master commit: e3c7eb185f0754862d49d186e08abd1df540cb84

cd mero
# Uncomment following line to compile mero with both KVS and Cassandra
# export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
