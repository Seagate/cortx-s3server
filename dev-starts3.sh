#!/bin/sh -xe
# Wrapper script to start S3 server in dev environment

export LD_LIBRARY_PATH="$(pwd)/third_party/mero/mero/.libs/:"\
"$(pwd)/third_party/mero/extra-libs/gf-complete/src/.libs/"

system/s3startsystem.sh
