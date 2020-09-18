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
please email opensource@seagate.com or cortx-questions@seagate.com.

## Introduction

This document describes some usage scenarios not covered in
[keepalived_config.md](keepalived_config.md) document.

-   IPv6 support
-   Node replacement
-   Removal of node
-   HAProxy monitoring in keepalived

## IPv6 support

`keepalived` supports IPv6. The only exceptions is `vrrp_instance::virtual_ipaddress`
should not contain mixed IPv4 and IPv6 records. However there are two ways to overcame
this limitation

-   create separate `vrrp_instance` for IPv4 and IPv6
-   use `vrrp_instance::virtual_ipaddress_excluded`

## Node replacement

Node replacement is a simple and straightforward operation. Assume we have a cluster
configured as described in [keepalived_config.md](keepalived_config.md)
with the use of helper scripts from the [keepalived_config_gen directory](../../scripts/keepalived/keepalived_config_gen).
Assume also the cluster is configured with the
[example_conf_3nodes_2ip.yaml](../../scripts/keepalived/keepalived_config_gen/example_conf_3nodes_2ip.yaml)
config and node `hw1` should be replaced with node `hw9`.

-   Stop node `hw1`

-   Edit [example_conf_3nodes_2ip.yaml](../../scripts/keepalived/keepalived_config_gen/example_conf_3nodes_2ip.yaml) config - change node record for `hw1` to node record for `hw9`
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

-   Generate config for the new node `hw9` as described in [README.md](../../scripts/keepalived/keepalived_config_gen/README.md)

    ```bash
    python3 keepalived_gen_mapping.py -o mapping_3n_2ip.yaml example_conf_3nodes_2ip.yaml
    python3 keepalived_gen_config.py  --host hw9 --yaml mapping_3n_2ip.yaml --conf keepalived.conf
    ```

-   Prepare node `hw9` - install `keepalived` and copy just created `keepalived.conf` to _/etc/keepalived/keepalived.conf_

-   Start the `keepalived` on node `hw9`

## Removal of node

Node could be removed from the `keepalived` cluster simply by switching it off.
However if associated virtual IPs should be removed from the cluster configuration
as well, it will require whole cluster reconfiguration from the scruch.

## HAProxy monitoring in keepalived

`keepalived` has an ability to monitor some external resources, e.g. files or processes,
and start failover based on the state of that resources.

`vrrp_script` field of the `keepalived` config should be used to monitor `HAProxy` process.
In the following example `vrrp_instance` with the name `VI_1` is configured to check `HAProxy`
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
