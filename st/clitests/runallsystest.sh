#!/bin/sh

set -e
abort()
{
    echo >&2 '
***************
*** ABORTED ***
***************
'
    echo "Error encountered. Exiting ST prematurely..." >&2
    trap : 0
    exit 1
}
trap 'abort' 0

sh ./prechecksystest.sh

python auth_spec.py
python s3cmd_spec.py
python jclient_spec.py
python jcloud_spec.py

echo >&2 '
**************************
*** ST Runs Successful ***
**************************
'
trap : 0
