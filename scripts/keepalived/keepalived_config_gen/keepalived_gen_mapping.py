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
import yaml

from keepalived_yaml_helper import IPMapping

def parse_cmd():
    parser = argparse.ArgumentParser(description='Generate YAML VIP to Hosts mapping')
    parser.add_argument('cfg', type=str,
                        help='YAML setup config path')

    parser.add_argument('-o', type=str, required=True, dest="output",
                        help='Output VIP to Host mapping')

    parser.add_argument('--node_list', type=str, required=False, default="",
                        help='Path to list of nodes. Overwrites params from config')

    parser.add_argument('--vip_list', type=str, required=False, default="",
                        help='Path to list of virtual IPs. Overwrites params from config')

    args = parser.parse_args()

    return args

def process_ip2host(nodes, ips, ip2h):
    ret_cfg = []
    for idx, m in enumerate(ip2h):
        cfg_nodes = list(map(int, m.split()))
        if len(cfg_nodes) != nodes:
            raise ValueError(f"Mapping does not describe configuration '{nodes} nodes, with {ips} IPs per node'")
        for cn in cfg_nodes:
            if 0 > cn or cn >= nodes:
                raise ValueError(f"Node index {cn} is outside the range [0, {nodes})")
        ret_cfg.append((idx, cfg_nodes[0], cfg_nodes[1:]))
    if len(ret_cfg) != nodes * ips:
        raise ValueError(f"Mapping does not describe configuration '{nodes} nodes, with {ips} IPs per node'")
    return ret_cfg

def process_list(exp_cnt, list_to_proc, dlm):
    ret_list = []
    for l in list_to_proc:
        if l:
            vals = l.strip().split(dlm)
            if len(vals) != 2:
                raise ValueError("Config should include 2 parts, separated with <{dlm}>")

        ret_list.append(vals)
    if len(ret_list) != exp_cnt:
        raise ValueError(f"Number of elements {len(ret_list)} is not equal to expected value {exp_cnt}")

    return ret_list

def read_all_lines(fpath):
    with open(fpath, "r") as fr:
        return fr.readlines()

def main():
    args = parse_cmd()
    inp_cfg = None
    with open(args.cfg, "r") as icfg:
        inp_cfg = yaml.safe_load(icfg)

    node_list = []
    if "node_list" in inp_cfg:
        node_list = process_list(inp_cfg["total_nodes"],
                                 inp_cfg["node_list"], " ")
    if args.node_list:
        node_list = process_list(inp_cfg["total_nodes"],
                                 read_all_lines(args.node_list), " ")

    vip_list = []
    if "vip_list" in inp_cfg:
        vip_list = process_list(inp_cfg["total_nodes"] * inp_cfg["ips_p_node"],
                                inp_cfg["vip_list"], "/")
    if args.vip_list:
        vip_list = process_list(inp_cfg["total_nodes"] * inp_cfg["ips_p_node"],
                                read_all_lines(args.vip_list), "/")

    ip2h = process_ip2host(inp_cfg["total_nodes"], inp_cfg["ips_p_node"],
                           inp_cfg["ip2host"])

    ipm = IPMapping(inp_cfg["total_nodes"], inp_cfg["ips_p_node"],
                    inp_cfg["auth_pass"], inp_cfg["base_rid"])
    ipm.set_cfg(node_list, vip_list, ip2h)
    ipm.dump(args.output)


if __name__ == "__main__":
    main()
