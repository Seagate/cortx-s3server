# Keepalived usage scenarios

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

This document describes some usage scenarios not covered in
_doc/LB_and_HA/keepalived_config.md_ document.

-   IPv6 support
-   Node replacement
-   Removal of node
-   HAProxy monitoring in keepalived

## IPv6 support

_keepalived_ supports IPv6. The only exceptions is _vrrp_instance::virtual_ipaddress_
should not contain mixed IPv4 and IPv6 records. However there are two ways to overcame
this limitation

-   create separate _vrrp_instance_ for IPv4 and IPv6
-   use _vrrp_instance::virtual_ipaddress_excluded_

## Node replacement

Node replacement is a simple and straightforward operation. Assume we have a cluster
configured as described in _doc/LB_and_HA/keepalived_config.md_
with the use of helper scripts _scripts/keepalived/keepalived_config_gen_. Assume also
the cluster is configured with the _scripts/keepalived/keepalived_config_gen/example_conf_3nodes_2ip.yaml_
config and node _hw1_ should be replaced with node _hw9_.

-   Stop node _hw1_

-   Edit _example_conf_3nodes_2ip.yaml_ config - change node record for _hw1_ to node record for _hw9_
```yml
node_list:
  - hw0 eth0
  - hw1 eth1
  - hw2 eth2
```
change to
```yml
node_list:
  - hw0 eth0
  - hw9 eth9
  - hw2 eth2
```

-   Generate config for the new node _hw9_ as described in _scripts/keepalived/keepalived_config_gen/README.md_

`python3 keepalived_gen_mapping.py -o mapping_3n_2ip.yaml example_conf_3nodes_2ip.yaml`  
`python3 keepalived_gen_config.py  --host hw9 --yaml mapping_3n_2ip.yaml --conf keepalived.conf`

-   Prepare node _hw9_ - install _keepalived_ and copy just created _keepalived.conf_ to _/etc/keepalived/keepalived.conf_

-   Start the _keepalived_ on node _hw9_

## Removal of node

Node could be removed from the _keepalived_ cluster simply by switching it off.
However if associated virtual IPs should be removed from the cluster configuration
as well, it will require whole cluster reconfiguration from the scruch.

## HAProxy monitoring in keepalived

_keepalived_ has an ability to monitor some external resources, e.g. files or processes,
and start failover based on the state of that resources.

_vrrp_script_ field of the _keepalived_ config should be used to monitor _HAProxy_ process.
In the following example _vrrp_instance_ with the name _VI_1_ is configured to check _HAProxy_
process status with the `killall -0 haproxy` command. This command will be run each 2 seconds.

```ini
vrrp_script check_haproxy {
script "killall -0 haproxy"
interval 2
}

vrrp_instance VI_1 {
    track_script {
        check_haproxy
    }
}
```
