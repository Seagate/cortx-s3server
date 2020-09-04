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

import yaml

class VIPAddr(yaml.YAMLObject):
    yaml_tag = u'!IPAddr'
    def __init__(self, ip, mask):
        self.vip = ip
        self.mask = mask
    def __repr__(self):
        return f"{self.vip}/{self.mask}"

class KAHost(yaml.YAMLObject):
    yaml_tag = u'!KAHost'
    def __init__(self, hst, iface):
        self.host = hst
        self.iface = iface
    def __repr__(self):
        return f"{self.host}/{self.iface}"

class IP2Host(yaml.YAMLObject):
    yaml_tag = u'!IP2Host'
    def __init__(self, ip, mnode, backups):
        self.vip = ip
        self.main_node = mnode
        self.backups = backups
    def __repr__(self):
        return f"{self.__class__.__name__}(vip={self.vip}, main_node={self.main_node}, backups={self.backups})"

class IPMapping(yaml.YAMLObject):
    yaml_tag = u'!IPMapping'
    def __init__(self, nodes, ips, auth_pass, base_rid):
        self.total_nodes = nodes
        self.ips_p_node = ips
        self.mapping = []
        self.auth_pass = auth_pass
        self.base_rid = base_rid
    def __repr__(self):
        return f"{self.__class__.__name__}(total_nodes={self.total_nodes}, ips_p_node={self.ips_p_node}, mapping={self.mapping}, auth_pass={self.auth_pass}, base_rid={self.base_rid})"

    def set_cfg(self, node_list, vip_list, cfg):
        for c in cfg:
            self.mapping.append(IP2Host(
                ip=VIPAddr(vip_list[c[0]][0], vip_list[c[0]][1]),
                mnode=KAHost(node_list[c[1]][0], node_list[c[1]][1]),
                backups=[KAHost(node_list[i][0], node_list[i][1]) for i in c[2]]))

    def dump(self, dpath):
        fname = dpath if dpath.endswith(".yaml") else f"{dpath}.yaml"
        with open(fname, "w") as dest:
            dest.write(yaml.dump(self))

def parse_yaml(yaml_path):
    ret_mapping = None
    with open(yaml_path, "r") as yc:
        ret_mapping = yaml.load(yc, Loader=yaml.Loader)

    return ret_mapping
