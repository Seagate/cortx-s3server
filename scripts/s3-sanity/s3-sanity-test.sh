#!/bin/bash

#########################
# S3 sanity test script #
#########################

USAGE="USAGE: bash $(basename "$0") [--help]
Run S3 sanity test.
where:
    --help      display this help and exit

Operations performed:
  * Create Bucket
  * List Buckets
  * Put Object
  * Get Object
  * Delete Object
  * Delete Bucket"

case "$1" in
    --help )
        echo "$USAGE"
        exit 0
        ;;
esac

# Security credentialis
AK='AKIAJPINPFRBTPAYOGNA'
SK='ht8ntpB9DoChDrneKZHvPVTm+1mHbs7UdCyYZ5Hd'

CLIENT='jcloud/jcloudclient.jar'
BUCKET='seagatebucket'
OBJECT='32MB.data'

# Add test account which is required to run S3 sanity test
cd ldap
echo 'Adding account: s3_test to LDAP for sanity testing...'
sudo chmod +x add_test_account.sh
sudo ./add_test_account.sh
cd ..

echo "***** S3: SANITY TEST *****"

set -e
echo -e "\nList Buckets:"
java -jar $CLIENT ls -p -x $AK -y $SK

echo -e "\nCreate buckekt - '$BUCKET':"
java -jar $CLIENT mb "s3://$BUCKET" -p -x $AK -y $SK

# create a file
dd if=/dev/urandom of=$OBJECT bs=8M count=4 status=none
content_md5_before=$(md5sum $OBJECT)

echo -e "\nUpload object '$OBJECT' to '$BUCKET':"
java -jar $CLIENT put $OBJECT "s3://$BUCKET" -p -x $AK -y $SK

echo -e "\nList objects in '$BUCKET':"
java -jar $CLIENT ls "s3://$BUCKET" -p -x $AK -y $SK

echo -e "\nDownload object '$OBJECT' from '$BUCKET':"
java -jar $CLIENT get "s3://$BUCKET/$OBJECT" $OBJECT -p -x $AK -y $SK
content_md5_after=$(md5sum $OBJECT)

echo -en "\nData integrity check:"
[[ $content_md5_before == $content_md5_after ]] && echo 'Passed.' || echo 'Failed.'

echo -e "\nDelete object '$OBJECT' from '$BUCKET':"
java -jar $CLIENT del "s3://$BUCKET/$OBJECT" -p -x $AK -y $SK

echo -e "\nDelete bucket - '$BUCKET':"
java -jar $CLIENT rb "s3://$BUCKET/$OBJECT" -p -x $AK -y $SK
set +e

echo "\n***** S3: SANITY TEST SUCCESSFULLY COMPLETED *****"

# delete a file
rm -f $OBJECT

# delte test account
cd ldap
echo 'Deleting account: s3_test.'
sudo chmod +x delete_test_account.sh
sudo ./delete_test_account.sh
cd ..
