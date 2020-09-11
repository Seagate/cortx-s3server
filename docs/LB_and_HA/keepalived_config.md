# Keepalived Configuration

## License

Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   <http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For any questions about this software or licensing,
please email opensource@seagate.com or cortx-questions@seagate.com.

## Introduction

This document describes how to setup and configure *keepalived*, daemon that
could be used to manage virtual IPs and move these IPs accross servers in
cluster.

*keepalived* can be used to reach two main goals

- High availability
- Fair load distribution in case of failover

### High availability

*keepalived* uses VRRP protocol to manage which host controlls IP address at
the moment, it is called master. As long as master node goes down backup node
becomes active and starts serving the IP address. After the master node restored
it gains the IP address back. There could be more than 1 backup node.

### Fair load distribution in case of failover

Assume 3 node cluster. All nodes are active and serving clients request.
In case of failover of first node its IP address migrate to second node. As
a result this node will have two active IP address and will have to serve two
times more requests than the third node.

One possible solution is to assign several IP addresses to each node and for
each IP address assign different backup nodes.
In this case after failover all requests from a failed node will be spread
accross several nodes.

Even load distribution in case of failover requires N * (N - 1) IP addresses
for the cluster of N nodes.

There could be different configurations as well. For example
4 IP addresses per node for cluster of N nodes - in case of failover
this configuratons leads to 25% load increase for some nodes.
5 IP addresses per node for cluster of N nodes - in case of failover
this configuratons leads to 20% load increase for some nodes.

#### Example configuration 1 - 3 nodes, 2 IPs per node

Total 6 IP addreses

| IP     | Master Node | Backup 1 | Backup 2 |
| ------ | ----------- | -------- | -------- |
| IP1_23 | Node 1      | Node 2   | Node 3   |
| IP1_32 | Node 1      | Node 3   | Node 2   |
| IP2_13 | Node 2      | Node 1   | Node 3   |
| IP2_31 | Node 2      | Node 3   | Node 1   |
| IP3_12 | Node 3      | Node 1   | Node 2   |
| IP3_21 | Node 3      | Node 2   | Node 1   |

#### Example configuration 2 - 6 nodes, 5 IPs per node

Total 30 IP addresses

| IP        | Master Node | Backup 1 | Backup 2 | Backup 3 | Backup 4 | Backup 5 |
| --------- | ----------- | -------- | -------- | -------- | -------- | -------- |
| IP1_23456 | Node 1      | Node 2   | Node 3   | Node 4   | Node 5   | Node 6   |
| IP1_34562 | Node 1      | Node 3   | Node 4   | Node 5   | Node 6   | Node 2   |
| IP1_45623 | Node 1      | Node 4   | Node 5   | Node 6   | Node 2   | Node 3   |
| IP1_56234 | Node 1      | Node 5   | Node 6   | Node 2   | Node 3   | Node 4   |
| IP1_62345 | Node 1      | Node 6   | Node 2   | Node 3   | Node 4   | Node 5   |
| IP2_13456 | Node 2      | Node 1   | Node 3   | Node 4   | Node 5   | Node 6   |
| IP2_34561 | Node 2      | Node 3   | Node 4   | Node 5   | Node 6   | Node 1   |
| IP2_45613 | Node 2      | Node 4   | Node 5   | Node 6   | Node 1   | Node 3   |
| IP2_56134 | Node 2      | Node 5   | Node 6   | Node 1   | Node 3   | Node 4   |
| IP2_61345 | Node 2      | Node 6   | Node 1   | Node 3   | Node 4   | Node 5   |
| IP3_12456 | Node 3      | Node 1   | Node 2   | Node 4   | Node 5   | Node 6   |
| IP3_24561 | Node 3      | Node 2   | Node 4   | Node 5   | Node 6   | Node 1   |
| IP3_45612 | Node 3      | Node 4   | Node 5   | Node 6   | Node 1   | Node 2   |
| IP3_56124 | Node 3      | Node 5   | Node 6   | Node 1   | Node 2   | Node 4   |
| IP3_61245 | Node 3      | Node 6   | Node 1   | Node 2   | Node 4   | Node 5   |
| IP4_23156 | Node 4      | Node 2   | Node 3   | Node 1   | Node 5   | Node 6   |
| IP4_31562 | Node 4      | Node 3   | Node 1   | Node 5   | Node 6   | Node 2   |
| IP4_15623 | Node 4      | Node 1   | Node 5   | Node 6   | Node 2   | Node 3   |
| IP4_56231 | Node 4      | Node 5   | Node 6   | Node 2   | Node 3   | Node 1   |
| IP4_62315 | Node 4      | Node 6   | Node 2   | Node 3   | Node 1   | Node 5   |
| IP5_13426 | Node 5      | Node 1   | Node 3   | Node 4   | Node 2   | Node 6   |
| IP5_34261 | Node 5      | Node 3   | Node 4   | Node 2   | Node 6   | Node 1   |
| IP5_42613 | Node 5      | Node 4   | Node 2   | Node 6   | Node 1   | Node 3   |
| IP5_26134 | Node 5      | Node 2   | Node 6   | Node 1   | Node 3   | Node 4   |
| IP5_61342 | Node 5      | Node 6   | Node 1   | Node 3   | Node 4   | Node 2   |
| IP6_12453 | Node 6      | Node 1   | Node 2   | Node 4   | Node 5   | Node 3   |
| IP6_24531 | Node 6      | Node 2   | Node 4   | Node 5   | Node 3   | Node 1   |
| IP6_45312 | Node 6      | Node 4   | Node 5   | Node 3   | Node 1   | Node 2   |
| IP6_53124 | Node 6      | Node 5   | Node 3   | Node 1   | Node 2   | Node 4   |
| IP6_31245 | Node 6      | Node 3   | Node 1   | Node 2   | Node 4   | Node 5   |

#### Example configuration 3 - 6 nodes, 2 IPs per node

Total 12 IP addresses

| IP        | Master Node | Backup 1 | Backup 2 | Backup 3 | Backup 4 | Backup 5 |
| --------- | ----------- | -------- | -------- | -------- | -------- | -------- |
| IP1_23456 | Node 1      | Node 2   | Node 3   | Node 4   | Node 5   | Node 6   |
| IP1_34562 | Node 1      | Node 3   | Node 4   | Node 5   | Node 6   | Node 2   |
| IP2_45613 | Node 2      | Node 4   | Node 5   | Node 6   | Node 1   | Node 3   |
| IP2_56134 | Node 2      | Node 5   | Node 6   | Node 1   | Node 3   | Node 4   |
| IP3_61245 | Node 3      | Node 6   | Node 1   | Node 2   | Node 4   | Node 5   |
| IP3_12456 | Node 3      | Node 1   | Node 2   | Node 4   | Node 5   | Node 6   |
| IP4_23561 | Node 4      | Node 2   | Node 3   | Node 5   | Node 6   | Node 1   |
| IP4_35613 | Node 4      | Node 3   | Node 5   | Node 6   | Node 1   | Node 3   |
| IP5_46123 | Node 5      | Node 4   | Node 6   | Node 1   | Node 2   | Node 3   |
| IP5_61234 | Node 5      | Node 6   | Node 1   | Node 2   | Node 3   | Node 4   |
| IP6_12345 | Node 6      | Node 1   | Node 2   | Node 3   | Node 4   | Node 5   |
| IP6_23451 | Node 6      | Node 2   | Node 3   | Node 4   | Node 5   | Node 1   |

## Install

This command installs *keepalived*

`#> yum install keepalived`

## Configuration

*keepalived* has a single configuration file
***/etc/keepalived/keepalived.conf***

There are two types of nodes in terms of *keepalived* - *master node* and *backup node*.
Each type of node has it own config. As a result config files for different machines
are different.

Configuration is done in terms of *vrrp_instance*. It describes the set of virtual ips
configured for the node. Only one node could be a *master* across a cluster for the
same *vrrp_instance*. The order *backup* nodes failover ip addresses is determined by
the *priority* field - next active node is the node with the highest *priority*. Each
*vrrp_instance* must have unique *virtual_router_id* in scope of a single sub-network.

### Configuration for [Example 1](#Example-configuration-1-3-nodes,-2-IPs-per-node)

Configuring cluster consists of 3 nodes, each node has 2 IP addresses

#### Node 1

***/etc/keepalived/keepalived.conf***

```keepalived
global_defs {
  enable_script_security
}

vrrp_instance VI_1_2_3 {
        state MASTER
        interface eth1
        virtual_router_id 77
        priority 110
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP1_23>/<netmask> dev eth1
        }
}

vrrp_instance VI_1_3_2 {
        state MASTER
        interface eth1
        virtual_router_id 78
        priority 110
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP1_32>/<netmask> dev eth1
        }
}

vrrp_instance VI_2_1_3 {
        state BACKUP
        interface eth1
        virtual_router_id 79
        priority 109
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP2_13>/<netmask> dev eth1
        }
}

vrrp_instance VI_2_3_1 {
        state BACKUP
        interface eth1
        virtual_router_id 80
        priority 108
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP2_31>/<netmask> dev eth1
        }
}

vrrp_instance VI_3_1_2 {
        state BACKUP
        interface eth1
        virtual_router_id 81
        priority 108
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP3_12>/<netmask> dev eth1
        }
}

vrrp_instance VI_3_2_1 {
        state BACKUP
        interface eth1
        virtual_router_id 82
        priority 109
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP3_21>/<netmask> dev eth1
        }
}
```

### Node 2

***/etc/keepalived/keepalived.conf***

```keepalived
global_defs {
  enable_script_security
}

vrrp_instance VI_1_2_3 {
        state BACKUP
        interface eth1
        virtual_router_id 77
        priority 109
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP1_23>/<netmask> dev eth1
        }
}

vrrp_instance VI_1_3_2 {
        state BACKUP
        interface eth1
        virtual_router_id 78
        priority 108
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP1_32>/<netmask> dev eth1
        }
}

vrrp_instance VI_2_1_3 {
        state MASTER
        interface eth1
        virtual_router_id 79
        priority 110
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP2_13>/<netmask> dev eth1
        }
}

vrrp_instance VI_2_3_1 {
        state MASTER
        interface eth1
        virtual_router_id 80
        priority 110
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP2_31>/<netmask> dev eth1
        }
}

vrrp_instance VI_3_1_2 {
        state BACKUP
        interface eth1
        virtual_router_id 81
        priority 109
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP3_12>/<netmask> dev eth1
        }
}

vrrp_instance VI_3_2_1 {
        state BACKUP
        interface eth1
        virtual_router_id 82
        priority 108
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP3_21>/<netmask> dev eth1
        }
}
```

### Node 3

***/etc/keepalived/keepalived.conf***

```keepalived
global_defs {
  enable_script_security
}

vrrp_instance VI_1_2_3 {
        state BACKUP
        interface eth1
        virtual_router_id 77
        priority 108
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP1_23>/<netmask> dev eth1
        }
}

vrrp_instance VI_1_3_2 {
        state BACKUP
        interface eth1
        virtual_router_id 78
        priority 109
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP1_32>/<netmask> dev eth1
        }
}

vrrp_instance VI_2_1_3 {
        state BACKUP
        interface eth1
        virtual_router_id 79
        priority 108
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP2_13>/<netmask> dev eth1
        }
}

vrrp_instance VI_2_3_1 {
        state BACKUP
        interface eth1
        virtual_router_id 80
        priority 109
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP2_31>/<netmask> dev eth1
        }
}

vrrp_instance VI_3_1_2 {
        state MASTER
        interface eth1
        virtual_router_id 81
        priority 110
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP3_12>/<netmask> dev eth1
        }
}

vrrp_instance VI_3_2_1 {
        state MASTER
        interface eth1
        virtual_router_id 82
        priority 110
        advert_int 1
        lvs_sync_daemon_interface eth1
        garp_master_delay 1

        track_interface {
              eth1
        }

        authentication {
              auth_type PASS
              auth_pass 12345
        }

        virtual_ipaddress {
              <IP3_21>/<netmask> dev eth1
        }
}
```

## Run Keepalived

`#> systemctl enable keepalived`  
`#> systemctl start keepalived`

## Testing Failover Scenarios

1. Run keepalived on all nodes. Ensure it started fine

    `#> systemctl status keepalived`

    Check configured IPs on all nodes

    `#> ip a`

    Node 1 should have IP1_23 and IP1_32,  
    Node 2 should have IP2_13 and IP2_31,  
    Node 3 should have IP3_12 and IP3_21  

2. Stop keepalived on one of the nodes, e.g. node 2 (or node 2 can be completely
switched off, etc)

    `#> systemctl stop keepalived`

    Or bring down the interface

    `#> ip link set dev eth1 down` or `#> ifconfig eth1 down`

    Check configured IPs on all nodes

    `#> ip a`

    Node 1 should have IP1_23, IP1_32 and IP2_13,  
    Node 3 should have IP3_12, IP3_21 and IP2_31,  
    Node 2 should not have any IP from the config list

3. Stop keepalived on one more node, e.g. node 1 (or switch it off, etc)

    `#> systemctl stop keepalived`

    Or bring down the interface

    `#> ip link set dev eth1 down` or `#> ifconfig eth1 down`

    Check configured IPs on all nodes

    `#> ip a`

    Node 3 should have all the IPs from the list,
    Node 1 and 2 should not have any IPs from the list

4. Restore one node and check the same way that IPs are reconfigured as in step 2

5. Restore all nodes and check the same way that IPs are reconfigured as in step 1

***Note:*** it is nice to have a ping command running during the test to
validate that IPs are configured properly
