#!/usr/bin/env python3

#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#

import argparse
from os import path

import keepalived_yaml_helper

def parse_cmd():
    parser = argparse.ArgumentParser(description='Generate keepalived config file')
    parser.add_argument('--host', type=str, required=True,
                        help='Host to generate config for')
    parser.add_argument('--yaml', type=str, required=True,
                        help='YAML mapping file')
    parser.add_argument('--conf', type=str, required=True,
                        help='Output keepalived config file')
    args = parser.parse_args()

    if not path.isfile(args.yaml):
        raise ValueError('YAML mapping is not accessible')

    return args

def vrrp_inst(name, state, iface, rid, prior, auth_pass, vip, vip_mask):
    tmpl = f"""
vrrp_instance {name} {{
        state {state}
        interface {iface}
        virtual_router_id {rid}
        priority {prior}
        advert_int 1
        lvs_sync_daemon_interface {iface}
        garp_master_delay 1

        track_interface {{
              {iface}
        }}

        authentication {{
              auth_type PASS
              auth_pass {auth_pass}
        }}

        virtual_ipaddress {{
              {vip}/{vip_mask} dev {iface}
        }}
}}
"""
    return tmpl

def get_state(h, i2h):
    return "MASTER" if h == i2h.main_node.host else "BACKUP"

def main():
    args = parse_cmd()
    base_prior = 200
    vipm = keepalived_yaml_helper.parse_yaml(args.yaml)
    res_cfg = """
global_defs {
  enable_script_security
}
"""
    for idx, m in enumerate(vipm.mapping):
        name = f"vip_inst_{idx}"
        rid = vipm.base_rid + idx
        state = get_state(args.host, m)
        prior = base_prior
        chost = m.main_node
        if state == "BACKUP":
            if args.host not in [b.host for b in m.backups]:
                raise ValueError(f"Host {args.host} is not listed in YAML mapping")
            for b in m.backups:
                prior -= 1
                if args.host == b.host:
                    chost = b
                    break

        res_cfg += vrrp_inst(name, state, chost.iface,
                             rid, prior, vipm.auth_pass,
                             m.vip.vip, m.vip.mask)

    with open(args.conf, "w") as cnf:
        cnf.write(res_cfg)


if __name__ == "__main__":
    main()
