## Steps to install & configure RabbitMQ

1) Install rabbitmq (On all nodes)
    Ref: https://www.rabbitmq.com/install-rpm.html#package-cloud
    Ref:https://packagecloud.io/rabbitmq/rabbitmq-server/install#bash-rpm


   >curl -s https://packagecloud.io/install/repositories/rabbitmq/rabbitmq-server/script.rpm.sh | sudo bash

2) Install pika on all nodes( BSD 3-Clause license)

    >yum --enablerepo=epel -y install python-pika

    or
    >yum --enablerepo=epel -y install python2-pika

3) Setup hosts file (/etc/hosts) on each node, so that nodes are
   accessible from each other
----
    cat /etc/hosts
    127.0.0.1   localhost localhost.localdomain localhost4 localhost4.localdomain4
    ::1         localhost localhost.localdomain localhost6 localhost6.localdomain6
    192.168.64.168 s3dev-client
    192.168.64.170 s3dev
----

4) Start rabbitmq (On all nodes)

   >systemctl start rabbitmq-server

5) Automatically start RabbitMQ at boot time 9On all nodes)

   >systemctl enable rabbitmq-server

6) Open up some ports, if we have firewall running (On all nodes)


   >firewall-cmd --zone=public --permanent --add-port=4369/tcp

   >firewall-cmd --zone=public --permanent --add-port=25672/tcp

   >firewall-cmd --zone=public --permanent --add-port=5671-5672/tcp

   >firewall-cmd --zone=public --permanent --add-port=15672/tcp

   >firewall-cmd --zone=public --permanent --add-port=61613-61614/tcp

   >firewall-cmd --zone=public --permanent --add-port=1883/tcp

   >firewall-cmd --zone=public --permanent --add-port=8883/tcp

   Reload the firewall

   >firewall-cmd --reload

7) Enable RabbitMQ Web Management Console (On all nodes)

   >rabbitmq-plugins enable rabbitmq_management

8) Setup RabbitMQ Cluster (Assuming there are 2 nodes s3dev and s3dev-client)

    In order to setup the RabbitMQ cluster, we need to make sure the '.erlang.cookie' file is same on all nodes.

    We will copy the '.erlang.cookie' file in the '/var/lib/rabbitmq' directory from 's3dev' to other node
    's3dev-client'.


    Copy the '.erlang.cookie' file using scp commands from the 's3dev' node.

   >scp /var/lib/rabbitmq/.erlang.cookie root@s3dev-client:/var/lib/rabbitmq/


9) Run below commands from 's3dev-client' node (node that will join s3dev node)

   >systemctl restart rabbitmq-server
   >rabbitmqctl stop_app

    Let RabbitMQ server on s3dev-client node, join s3dev node

   >rabbitmqctl join_cluster rabbit@s3dev
   >rabbitmqctl start_app

    check the RabbitMQ cluster status on both nodes

   >rabbitmqctl cluster_status


10) Create a user (In this case user is admin with password as password)

    >rabbitmqctl add_user admin password


11) Setup the admin user as an administrator.

    >rabbitmqctl set_user_tags admin administrator


11) Set permission

    >rabbitmqctl set_permissions -p / admin ".*" ".*" ".*"


12) Check users on cluster nodes (all nodes will list them)
    >rabbitmqctl list_users


13) Setup queue mirroring

    >rabbitmqctl set_policy ha-all ".*" '{"ha-mode":"all"}'


14) Tests
    Open your web browser and type the IP address of the node with port '15672'.

    >http://192.168.64.170:15672/

    Type username 'admin' with password 'password'

    Note:Ensure that port 15672 is open