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


ldapdelete -x -w seagate -r "ak=AKIAJPINPFRBTPAYOGNA,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=admin,dc=seagate,dc=com"

ldapdelete -x -w seagate -r "ak=AKIAJTYX36YCKQSAJT7Q,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=admin,dc=seagate,dc=com"

ldapdelete -x -w seagate -r "o=s3_test,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=admin,dc=seagate,dc=com"
