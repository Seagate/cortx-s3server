#!/bin/sh
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
AUTH_INSTALL_PATH="/opt/seagate/cortx/auth"
DEV_VM_JKS_DIR="/root/.cortx_s3_auth_jks"
AUTH_KEY_ALIAS="s3auth_pass"
DEFAULT_KEYSTORE_PASSWD="seagate"
DEFAULT_KEY_PASSWD="seagate"

if [[ "$BASEDIR" == "$AUTH_INSTALL_PATH/scripts" ]];
then
    # this is executed for cluster deployment
    AUTH_KEYSTORE_PROPERTIES_FILE="$AUTH_INSTALL_PATH/resources/keystore.properties"
    AUTH_JKS_TEMPLATE_FILE="$AUTH_INSTALL_PATH/scripts/s3authserver.jks_template"
    AUTH_JKS_FILE="$AUTH_INSTALL_PATH/resources/s3authserver.jks"
    cp -f $AUTH_JKS_TEMPLATE_FILE $AUTH_JKS_FILE
else
    # this script executed for dev vm
    mkdir -p $DEV_VM_JKS_DIR
    cp -f $BASEDIR/s3authserver.jks_template $DEV_VM_JKS_DIR/s3authserver.jks
    cp -f $BASEDIR/../auth/resources/keystore.properties.sample $DEV_VM_JKS_DIR/keystore.properties
    AUTH_KEYSTORE_PROPERTIES_FILE=$DEV_VM_JKS_DIR/keystore.properties
    AUTH_JKS_FILE=$DEV_VM_JKS_DIR/s3authserver.jks
fi

# Generate random password for jks keystore
generate_keystore_password(){
  echo "Generating password for jks keystore used in authserver..."
  # Get password from cortx-utils
  new_keystore_passwd=$(s3cipher --use_base64 --key_len  12  --const_key  openldap 2>/dev/null)
  if [[ $? != 0 || -z "$new_keystore_passwd" ]] # Generate random password if failed to get from cortx-utils
  then
    new_keystore_passwd=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9@#+' | fold -w 12 | head -n 1)
    # Update keystore password
    # Note JKS store need same password for store password and key password otherwise JKS will not work
    # Update keystore.properties file with new password
    sudo sed -i 's/s3KeyStorePassword=.*$/s3KeyStorePassword='$new_keystore_passwd'/g' $AUTH_KEYSTORE_PROPERTIES_FILE
    sudo sed -i 's/s3KeyPassword=.*$/s3KeyPassword='$new_keystore_passwd'/g' $AUTH_KEYSTORE_PROPERTIES_FILE
  fi
  
  # Update keystore password
  # Note JKS store need same password for store password and key password otherwise JKS will not work
  keytool -storepasswd -storepass $DEFAULT_KEYSTORE_PASSWD -new $new_keystore_passwd -keystore $AUTH_JKS_FILE
  keytool -keypasswd --keypass $DEFAULT_KEY_PASSWD -new $new_keystore_passwd -alias $AUTH_KEY_ALIAS -storepass $new_keystore_passwd --keystore $AUTH_JKS_FILE
  keytool -delete -alias $AUTH_KEY_ALIAS -keystore $AUTH_JKS_FILE -storepass $new_keystore_passwd -keypass $new_keystore_passwd
  keytool -genkeypair -keyalg RSA -alias $AUTH_KEY_ALIAS -keystore $AUTH_JKS_FILE -storepass $new_keystore_passwd -keypass $new_keystore_passwd -validity 3600 -keysize 512 -dname "C=IN, ST=Maharashtra, L=Pune, O=Seagate, OU=S3, CN=iam.seagate.com"
  echo "Updated the JKS Keystore."
}
generate_keystore_password
