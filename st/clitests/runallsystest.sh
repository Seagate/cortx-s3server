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

source mero_st/bin/activate

echo "`date -u`: Running auth_spec.py..."
python auth_spec.py --all

echo "`date -u`: Running s3cmd_spec.py..."
python s3cmd_spec.py

echo "`date -u`: Running jclient_spec.py..."
python jclient_spec.py

echo "`date -u`: Running jcloud_spec.py..."
python jcloud_spec.py

echo "`date -u`: Running rollback_spec.py..."
python rollback_spec.py

echo "`date -u`: Running negative_spec.py..."
python negative_spec.py

echo "`date -u`: Running shutdown_spec.py..."
python shutdown_spec.py

# undo the virtualenv
deactivate

echo >&2 '
**************************
*** ST Runs Successful ***
**************************
'
trap : 0
