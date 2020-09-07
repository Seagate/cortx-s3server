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
please email opensource@seagate.com or cortx-questions@seagate.com.

## Introduction

This folder contains scripts that could be used to generate proper keepalived
config files for all nodes in the cluster.

Config generation is done in two steps

- create mapping of virtual IPs to hosts
- generate actual keepalived configs based on the mapping

## Folder structure

- *keepalived_yaml_helper.py* - contains wrappers to dump/load mapping description
to/from yaml file
- *keepalived_gen_mapping.py* - generates mapping file for actual keepalived
config generation
- *example_conf_3nodes_2ip.yaml* - example config file for mapping generation
based on 3 node cluster with 2 IPs per node
- *keepalived_gen_config.py* - generates keepalived config file from ip to host
mapping

## Requirements

- python3.6
- PyYAML 5.1.2

## Create mapping of virtual IPs to hosts

Following command will generate mapping file *mapping_3n_2ip.yaml* based on
parameters provided in *example_conf_3nodes_2ip.yaml* config file

`python3 keepalived_gen_mapping.py -o mapping_3n_2ip.yaml example_conf_3nodes_2ip.yaml`

Command line parameters:

- *-o* - path to output YAML file
- *--node_list* - Optional. Allows to overwrite *node_list* paramter from config
- *--vip_list* - Optional. Allows to overwrite *vip_list* paramter from config
- *cfg* - Path to YAML config file

### Config file format description

Config file must be a valid YAML file with the following allowed parameters

- *total_nodes* - Int. Total number of nodes in cluster
- *ips_p_node* - Int. Number of IPs per each node
- *auth_pass* - Str. auth_pass param for keepalived vrrp_instance
- *base_rid* - Int. Keepalived virtual_router_id param value to start with.
vrrp_instances::virtual_router_id is equal *base_rid* + vrrp idx
- *node_list* - List of Str. List of nodes in form of \<host\> \<iface to config\>.
Must contain total_nodes items. Could be skipped, in this case ***--node_list***
command line paramters must be provided
- *vip_list* - List of Str. List of virtual IP addresses to configure in form
of \<ip\>/\<net mask\>. Must contain *total_nodes* **ips_p_node* items. Could be
skiiped, in this case***--vip_list*** command line param must be provided
- *ip2host* - List of Int. Describes mapping of virtula IPs to hosts. Each line
describes a single virtual IP from the ***vip_list***. Nodes are referenced as
zero based index from ***node_list***.
Format: \<main node\> \<backup 1\> \<backup 2\> ... \<backup n\>
Number of backups should be equal to ***total_nodes - 1***

#### example_conf_3nodes_2ip.yaml

```yaml
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
```

## Generate actual keepalived configs based on the mapping

Following command will generate *keepalived.conf* config file for the *hw0* node
based on generted mapping file *mapping_3n_2ip.yaml*

`python3 keepalived_gen_config.py  --host hw0 --yaml mapping_3n_2ip.yaml --conf keepalived.conf`

Command line parameters:

- *--host* - Str. Name of the node to generate config for
- *--yaml* - Str. Path to mapping YAML file generated with *keepalived_gen_mapping.py* script
- *--conf* - Str. Path where generated config should be stored
