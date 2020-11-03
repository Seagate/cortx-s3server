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

#Steps to install kafka on a single node

1) Download kafka kit, say kafka_2.11-2.2.2.tgz is downloaded to /opt/
2) cd /opt/ then tar -xvf kafka_2.11-2.2.2.tgz 
3) ln -s /opt/kafka_2.11-2.2.2 /opt/kafka
4) useradd kafka
5) chown -R kafka:kafka /opt/kafka*
6) create /etc/systemd/system/zookeeper.service and /etc/systemd/system/kafka.service files with necessary entries
7) systemctl daemon-reload
8) systemctl start zookeeper
9) systemctl start kafka

Create topic
-----------
/opt/kafka/bin/kafka-topics.sh --create --topic s3bgd --bootstrap-server localhost:9092
