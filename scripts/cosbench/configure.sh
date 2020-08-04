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
read -p "Enter the label for S3 setup:" s3_setup_label
read -p "S3 endpoint url:" s3_url_endpoint
read -p "Access Key:" s3_access_key
read -p "Secret Key:" s3_secret_key
s3_setup_label="$s3_setup_label.properties"
echo "s3_endpoint:$s3_url_endpoint" > "$s3_setup_label"
echo "access_key:$s3_access_key" >> "$s3_setup_label"
echo "secret_key:$s3_secret_key" >> "$s3_setup_label"
printf "\nCreated S3 setup properties file $s3_setup_label\n\n"
