#!/bin/sh

set -e
abort()
{
    echo >&2 '
***************
*** ABORTED ***
***************
'
    echo "Error encountered. Precheck failed..." >&2
    trap : 0
    exit 1
}
trap 'abort' 0

JCLIENTJAR='jclient.jar'
JCLOUDJAR='jcloudclient.jar'

printf "\nCheck cqlsh..."
type cqlsh >/dev/null
printf "OK "
if [ -f $JCLIENTJAR ] ;then
    printf "\nCheck $JCLIENTJAR...OK"
else
    printf "\nCheck $JCLIENTJAR...Not found"
    abort
fi
if [ -f $JCLOUDJAR ] ;then
    printf "\nCheck $JCLOUDJAR...OK"
else
    printf "\nCheck $JCLOUDJAR...Not found"
    abort
fi
printf "\nCheck seagate host entries for system test..."
getent hosts seagatebucket.s3.seagate.com seagate-bucket.s3.seagate.com >/dev/null
getent hosts seagatebucket123.s3.seagate.com seagate.bucket.s3.seagate.com >/dev/null
getent hosts iam.seagate.com sts.seagate.com >/dev/null
printf "OK \n"

trap : 0
