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
/opt/kafka/bin/kafka-console-producer.sh --broker-list localhost:9092 --topic s3bgs

