#!/bin/sh -xe
# Script to build mero.
# git repo: http://es-gerrit.xyus.xyratex.com:8080/mero
# branch: master commit: 2fddc657f43b54a179c009cb0ac7fbfffba6b61b

cd mero
export CONFIGURE_OPTS=--with-cassandra
./scripts/m0 rebuild
cd ..
