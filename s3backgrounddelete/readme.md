### License

Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For any questions about this software or licensing,
please email opensource@seagate.com or cortx-questions@seagate.com.

## Steps to configure & run S3backgrounddelete process
----
1. Setup and configure CortxS3 Dev VM as per

   > {s3 src}/docs/CORTX-S3 Server Quick Start Guide.md

2. Ensure/Start following services are running
    a. Haproxy
      > systemctl start haproxy

    b. Ldap
      > systemctl start slapd

    c. Authserver
      > systemctl start s3authserver

    d. Motr
      > {s3 src}/third_party/motr/m0t1fs/../motr/st/utils/motr_services.sh start

    e. S3server
      > ./dev-starts3.sh

3. Create special account for s3backgroundelete
    > python36 scripts/ldap/create_account_using_cipher.py CreateBGDeleteAccount --ldapuser 'sgiamadmin' --ldappasswd 'ldapadmin'

4.  Add proper scheduler_schedule_interval in

   > /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml

  for s3backgounddelete process. Default is 900 seconds.

5. Start S3backgrounddelete scheduler service
   > systemctl start s3backgroundproducer

6. Start S3backgrounddelete processor service
   > systemctl start s3backgroundconsumer

7. Use following scripts to generate stale oids in s3server
   > {s3 src}/s3backgrounddelete/scripts/delete_object_leak.sh
   > {s3 src}/s3backgrounddelete/scripts/update_object_leak.sh

8.  Check scheduler and processor logs at
   > /var/log/cortx/s3/s3backgrounddelete/object_recovery_scheduler.log &
   > /var/log/cortx/s3/s3backgrounddelete/object_recovery_processor.log respectively.

9.  For s3backgroudelete algorithm please refer
   > {s3 src}/docs/design/probable-delete-processing.txt &
   > {s3 src}/docs/design/probable_delete_record.txt
----
