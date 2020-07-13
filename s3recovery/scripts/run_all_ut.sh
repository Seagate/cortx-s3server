#!/bin/sh
# COPYRIGHT 2020 SEAGATE LLC

# THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
# HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
# LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
# THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
# BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
# USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
# EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.

# YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
# THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
# http://www.seagate.com/contact

# Original author: Amit Kumar  <amit.kumar@seagate.com>
# Original creation date: 08-July-2020

#Script to run UT's of s3recovery tool
set -e

abort()
{
    echo >&2 '
***************
*** FAILED ***
***************
'
    echo "Error encountered. Exiting unit test runs prematurely..." >&2
    trap : 0
    exit 1
}
trap 'abort' 0

printf "\nRunning s3recovery tool UT's...\n"

SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

#Update python path to source modules and run unit tests.
PYTHONPATH=${PYTHONPATH}:${SCRIPT_DIR}/.. python36 -m pytest $SCRIPT_DIR/../ut/*.py -v

echo "s3recovery tool UT's runs successfully"

trap : 0

