#!/bin/sh
set -x
git clone git@github.com:Seagate/cortx-motr.git
./cortx-motr/scripts/install-build-deps
rm -rf cortx-motr
