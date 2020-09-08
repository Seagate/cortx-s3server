# Keepalived Config Generation Scripts

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
please email [opensource@seagate.com](mailto:opensource@seagate.com) or
[cortx-questions@seagate.com](mailto:cortx-questions@seagate.com).

## Introduction

This folder contains scripts that could be used to generate proper keepalived
config files for all nodes in the cluster.

Config generation is done in two steps

-   create mapping of virtual IPs to hosts
-   generate actual keepalived configs based on the mapping

## Folder structure

-   _keepalived_yaml_helper.py_ - contains wrappers to dump/load mapping description
    to/from yaml file

-   _keepalived_gen_mapping.py_ - generates mapping file for actual keepalived
    config generation

-   _example_conf_3nodes_2ip.yaml_ - example config file for mapping generation
    based on 3 node cluster with 2 IPs per node

-   _keepalived_gen_config.py_ - generates keepalived config file from ip to host
    mapping

## Requirements

-   python3.6
-   PyYAML 5.1.2

## Create mapping of virtual IPs to hosts

Following command will generate mapping file _mapping_3n_2ip.yaml_ based on
parameters provided in _example_conf_3nodes_2ip.yaml_ config file

`python3 keepalived_gen_mapping.py -o mapping_3n_2ip.yaml example_conf_3nodes_2ip.yaml`

Command line parameters:

-   _-o_ - path to output YAML file
-   _--node_list_ - Optional. Allows to overwrite _node_list_ paramter from config
-   _--vip_list_ - Optional. Allows to overwrite _vip_list_ paramter from config
-   _cfg_ - Path to YAML config file

### Config file format description

Config file must be a valid YAML file with the following allowed parameters

-   _total_nodes_ - Int. Total number of nodes in cluster

-   _ips_p_node_ - Int. Number of IPs per each node

-   _auth_pass_ - Str. auth_pass param for keepalived vrrp_instance

-   _base_rid_ - Int. Keepalived virtual_router_id param value to start with.
    _vrrp_instances::virtual_router_id_ is equal to _base_rid + vrrp idx_. All the
    _vrrp_instances::virtual_router_id_ must be unique in scope of sub-network,
    so the _base_rid_ should be selected to be unique together with _base_rid + vrrp idx_
    for all vrrp_instances. If no other _keepalived_ services are run in scope of a
    sub-network _base_rid_ could be set to any value

-   _node_list_ - List of Str. List of nodes in form of &lt;host> &lt;iface to config>.
    Must contain _total_nodes_ items. Could be skipped, in this case _node_list_
    command line paramters must be provided

-   _vip_list_ - List of Str. List of virtual IP addresses to configure in form
    of &lt;ip>/&lt;net mask>. Must contain _total_nodes_ * _ips_p_node_ items. Could be
    skiiped, in this case _vip_list_ command line param must be provided

-   _ip2host_ - List of Int. Describes mapping of virtula IPs to hosts. Each line
    describes a single virtual IP from the _vip_list_. Nodes are referenced as
    zero based index from _node_list_.
    Format: &lt;main node> &lt;backup 1> &lt;backup 2> ... &lt;backup n>
    Number of backups should be equal to _total_nodes - 1_. Order backups are listed
    is important since in this order backups will failover IPs in case multiple nodes
    fail

#### example_conf_3nodes_2ip.yaml

    # Total number of nodes in cluster
    total_nodes: 3

    # IPs per each node
    ips_p_node: 2

    # auth_pass param for keepalived config
    auth_pass: ap123456

    # Keepalived virtual_router_id param value to start with
    # vrrp_instances::virtual_router_id is equal base_rid + vrrp idx
    base_rid: 77

    # List of nodes in form of <host> <iface to config>
    # Must contain total_nodes items
    node_list:
    - hw0 eth0
    - hw1 eth1
    - hw2 eth2

    # List of virtual IP addresses to configure in form of <ip>/<net mask>
    # Must contain total_nodes * ips_p_node items
    vip_list:
    - 192.168.1.0/20
    - 192.168.1.1/20
    - 192.168.1.2/20
    - 192.168.1.3/20
    - 192.168.1.4/20
    - 192.168.1.5/20

    # Describes mapping of virtula IPs to hosts
    # Each line describes a single virtual IP from the vip_list
    # Nodes are referenced as zero based index from node_list
    # format: <main node> <backup 1> <backup 2> ... <backup n>
    # number of backups should be equal to total_nodes - 1
    ip2host:
    - 0 1 2
    - 0 2 1
    - 1 0 2
    - 1 2 0
    - 2 0 1
    - 2 1 0

## Generate actual keepalived configs based on the mapping

Following command will generate _keepalived.conf_ config file for the _hw0_ node
based on generted mapping file _mapping_3n_2ip.yaml_

`python3 keepalived_gen_config.py  --host hw0 --yaml mapping_3n_2ip.yaml --conf keepalived.conf`

Command line parameters:

-   _--host_ - Str. Name of the node to generate config for
-   _--yaml_ - Str. Path to mapping YAML file generated with _keepalived_gen_mapping.py_ script
-   _--conf_ - Str. Path where generated config should be stored
