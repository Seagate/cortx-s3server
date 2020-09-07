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


set -e
SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
AUTH_INSTALL_PATH="/opt/seagate/cortx/auth"

# Set file path based on setup type i.e. provisioner/dev
if [[ "$BASEDIR" == "$AUTH_INSTALL_PATH/scripts" ]];
then
    AUTH_KEYSTORE_PROPERTIES_FILE="$AUTH_INSTALL_PATH/resources/keystore.properties"
    AUTH_JKS_TEMPLATE_FILE="$AUTH_INSTALL_PATH/scripts/s3authserver.jks_template"
    AUTH_JKS_FILE="$AUTH_INSTALL_PATH/resources/s3authserver.jks"
else
    AUTH_KEYSTORE_PROPERTIES_FILE="$BASEDIR/../auth/resources/keystore.properties"
    AUTH_JKS_TEMPLATE_FILE="$BASEDIR/s3authserver.jks_template"
    AUTH_JKS_FILE="$BASEDIR/../auth/resources/s3authserver.jks"
fi

AUTH_KEY_ALIAS="s3auth_pass"
DEFAULT_KEYSTORE_PASSWD="seagate"
DEFAULT_KEY_PASSWD="seagate"

#Use template file to change password
cp -f $AUTH_JKS_TEMPLATE_FILE $AUTH_JKS_FILE

# Generate random password for jks keystore
generate_keystore_password(){
  echo "Generating random password for jks keystore used in authserver..."
  # Generate random password
  new_keystore_passwd=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9@#+' | fold -w 12 | head -n 1)

  # Update keystore password
  # Note JKS store need same password for store password and key password otherwise JKS will not work
  keytool -storepasswd -storepass $DEFAULT_KEYSTORE_PASSWD -new $new_keystore_passwd -keystore $AUTH_JKS_FILE
  keytool -keypasswd --keypass $DEFAULT_KEY_PASSWD -new $new_keystore_passwd -alias $AUTH_KEY_ALIAS -storepass $new_keystore_passwd --keystore $AUTH_JKS_FILE

  # Update keystore.properties file with new password
  sudo sed -i 's/s3KeyStorePassword=.*$/s3KeyStorePassword='$new_keystore_passwd'/g' $AUTH_KEYSTORE_PROPERTIES_FILE
  sudo sed -i 's/s3KeyPassword=.*$/s3KeyPassword='$new_keystore_passwd'/g' $AUTH_KEYSTORE_PROPERTIES_FILE

  echo "jks keystore passwords are updated successfully...."
}

generate_keystore_password
