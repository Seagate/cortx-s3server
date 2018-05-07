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

#Using Python 3.4 version for Running System Tests
PythonV="python3.4"

echo "`date -u`: Running auth_spec.py..."
$PythonV auth_spec.py --all

echo "`date -u`: Running s3cmd_spec.py..."
$PythonV s3cmd_spec.py

echo "`date -u`: Running jclient_spec.py..."
$PythonV jclient_spec.py

echo "`date -u`: Running jcloud_spec.py..."
$PythonV jcloud_spec.py

echo "`date -u`: Running rollback_spec.py..."
$PythonV rollback_spec.py

echo "`date -u`: Running negative_spec.py..."
$PythonV negative_spec.py

echo "`date -u`: Running shutdown_spec.py..."
$PythonV shutdown_spec.py

echo >&2 '
**************************
*** ST Runs Successful ***
**************************
'
trap : 0
