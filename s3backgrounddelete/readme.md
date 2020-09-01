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
1. Install & configure RabbbitMQ (In LDR-R1) as per {s3 src}/s3backgrounddelete/rabbitmq_setup.md

2. Create s3backgrounddelete log folder
    >mkdir -p /var/log/seagate/s3/s3backgrounddelete

3. Start S3 and motr services after setting S3_SERVER_ENABLE_OBJECT_LEAK_TRACKING to true in
    /opt/seagate/cortx/s3/conf/s3config.yaml.

4. Create an account with account name s3-background-delete-svc and update
    access_key and secret_key in /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml

5. Use following scripts to generate stale oids in s3server
    >{s3 src}/s3backgrounddelete/scripts/delete_object_leak.sh
    {s3 src}/s3backgrounddelete/scripts/update_object_leak.sh

6. Add proper schedule_interval_secs in
    /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml for s3backgounddelete process. Default is 900 seconds.

7. Start S3backgrounddelete scheduler service

    >systemctl start s3backgroundproducer

8. Start S3backgrounddelete processor service

    >systemctl start s3backgroundconsumer

9.  Check scheduler and processor logs at
    >/var/log/seagate/s3/s3backgrounddelete/object_recovery_scheduler.log &
    >/var/log/seagate/s3/s3backgrounddelete/object_recovery_processor.log respectively.
----
