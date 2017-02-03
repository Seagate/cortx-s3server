#!/bin/sh -xe
# Script to build mero.
# git repo: http://es-gerrit.xyus.xyratex.com:8080/mero
# branch: dev/clovis2 commit: 1838c46ef38d0d3a3558706c620b4a581a6b7159

cd mero
./scripts/m0 rebuild
cd ..
