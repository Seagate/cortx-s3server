#!/bin/sh

# This script installs virtual env and creates a virtual env for python3.5
# in the user's current directory
#
# It also installs all the dependencies required for pyclient.

pip install virtualenv
virtualenv -p /usr/local/bin/python3.5 mero_st
source mero_st/bin/activate
pip install python-dateutil==2.4.2
pip install pyyaml==3.11
pip install xmltodict==0.9.2
pip install boto3==1.2.2
pip install botocore==1.3.8

echo ""
echo ""
echo "Add the following to /etc/hosts"
echo "127.0.0.1 iam.seagate.com sts.seagate.com"
echo "127.0.0.1 s3.seagate.com"
