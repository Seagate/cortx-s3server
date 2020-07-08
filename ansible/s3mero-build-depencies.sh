#!/bin/sh
set -x

SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

motr_script_path=$SCRIPT_DIR/../third_party/mero/scripts
if [[ -d "$motr_script_path" && -f "$motr_script_path/install-build-deps" ]];
then
    sh $motr_script_path/install-build-deps
else
    # third_party submodules are not loaded yet hence do fresh clone and
    # run install-build-deps script.
    # This needs GitHub id and Personal Access Token
    git clone https://github.com/Seagate/cortx-motr.git
    ./cortx-motr/scripts/install-build-deps
    rm -rf cortx-motr
fi
