#!/bin/sh -x

rm -rf mero
git clone -b dev/clovis2 --recursive --single-branch http://es-gerrit.xyus.xyratex.com:8080/mero
cd mero
git checkout 1838c46ef38d0d3a3558706c620b4a581a6b7159
./scripts/m0 rebuild
cd ..
