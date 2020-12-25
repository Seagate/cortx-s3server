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


#################################################
# Script to generate dns round robin config files
#################################################

import argparse
import sys
import os
from typing import List

def chk_ip(ip: str) -> bool:
    octs = ip.split(".")
    if len(octs) != 4:
        return False
    for o in octs:
        try:
            ioct = int(o)
            if ioct > 255 or ioct < 0:
                return False
        except ValueError:
            return False
    return True

def chk_dir(d: str) -> bool:
    return os.path.isdir(d) and os.access(d, os.W_OK)


def parse_args():
    parser = argparse.ArgumentParser(
        prog=sys.argv[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""Creates configuration files for BIND to round robin IPs.""",
        epilog=f"""Generated configs should be copied to
name.conf -> /etc/
forward.<zone> -> /var/named/
reverse.<zone> -> /var/named/


Examples
To generate configs with default params: zone seagate.com and services s3, iam
        #> {sys.argv[0]} --ip 192.168.1.0 192.168.1.1

To generate configs for different zones and services
        #> {sys.argv[0]} --ip 192.168.1.0 192.168.1.1 --zone test.zone --services srv1 srv2

To install BIND use following command
        #> yum install bind bind-utils

To run BIND use following commands
        #> systemctl enable named.service
        #> systemctl start named.service

To restart BIND after config update use following command
        #> systemctl restart named.service
        """)

    argz = parser.add_argument("-z", "--zone", type=str, required=False, default="seagate.com",
                               help="Name of the zone to create")
    argsrvs = parser.add_argument("-s", "--services", nargs="+", type=str, default=["s3", "iam"],
                                  required=False,
                                  help="Services that should be created inside zone and mapped to IPs")
    argip = parser.add_argument("-i", "--ip", nargs="+", type=str, default=[],
                                help="IP addresses")
    argout = parser.add_argument("-o", "--outdir", type=str, required=False, default=".",
                                 help="Directory to store config files")

    args = parser.parse_args()

    if not args.ip:
        raise argparse.ArgumentError(argip, "At least one IP should be provided")

    for ip in args.ip:
        if not chk_ip(ip):
            raise argparse.ArgumentError(argip, "Invalid IP adress format")

    if not chk_dir(args.outdir):
        raise argparse.ArgumentError(argout, "outdir is not a directory with write access")

    if not args.services:
        raise argparse.ArgumentError(argsrvs, "At least one services should be provided")

    for s in args.services:
        if not s:
            raise argparse.ArgumentError(argsrvs, "Invalid service name")

    if not args.zone:
        raise argparse.ArgumentError(argz, "Invalid zone name")

    return args

def reverse_ip(ip: str) -> str:
    iocts = reversed(list(map(int, ip.split("."))))
    return ".".join(map(str, iocts))

def straight_ip(ip: str) -> str:
    iocts = (map(int, ip.split(".")))
    return ".".join(map(str, iocts))

def gen_named_conf(zone: str, ips: List[str], odir: str):
    basecfg = r'''options {
 listen-on port 53 { 127.0.0.1; };
 listen-on-v6 port 53 { ::1; };
 directory  "/var/named";
 dump-file  "/var/named/data/cache_dump.db";
 statistics-file "/var/named/data/named_stats.txt";
 memstatistics-file "/var/named/data/named_mem_stats.txt";
 recursing-file  "/var/named/data/named.recursing";
 secroots-file   "/var/named/data/named.secroots";

 /* This should be changed to "localhost" to process */
 /* requests from localhost only */
 allow-query     { localhost; };

 /* This should be changed to "no" to limit requests with local server only */
 recursion no;

 dnssec-enable yes;
 dnssec-validation yes;

 /* Path to ISC DLV key */
 bindkeys-file "/etc/named.root.key";

 managed-keys-directory "/var/named/dynamic";

 pid-file "/run/named/named.pid";
 session-keyfile "/run/named/session.key";

 /* This param should be added to process all the records in cyclic order */
 rrset-order { order cyclic; };
};

logging {
        channel default_debug {
                file "data/named.run";
                severity dynamic;
        };
};

zone "." IN {
 type hint;
 file "named.ca";
};

include "/etc/named.rfc1912.zones";
include "/etc/named.root.key";

'''

    forward_tmpl = f'''// This should be added to provide Forward zone description
zone "{zone}"  {{
        type master;
        file    "/var/named/forward.{zone}";
}};

'''

    reverse_tmpl = '''// This should be added to provide Reverse zone description
zone   "{revip}.in-addr.arpa"  {{
       type master;
       file    "/var/named/reverse.{zone}";
}};

'''

    with open(os.path.join(odir, "named.conf"), "w") as nmdc:
        nmdc.write(basecfg)
        nmdc.write(forward_tmpl)
        for ip in ips:
            revip = reverse_ip(ip)
            nmdc.write(reverse_tmpl.format(revip=revip, zone=zone))

def gen_forward_conf(zone: str, ips: List[str], srvs: List[str], odir: str):
    base_tmpl = f'''$TTL 1s

@               IN      SOA     dns1.{zone}.    hostmaster.{zone}. (
                1        ; serial
                1m       ; refresh after
                12s      ; retry after
                1w       ; expire after
                5s )     ; minimum TTL
;
;
;Name Server Information
@               IN      NS      dns1.{zone}.
dns1            IN      A       127.0.0.1

;
; A Records:
; Configuring IP addresses should be added here
'''
    a_rec_tmpl = "{srv}             IN      A       {ip}\n"

    cname_rec_tmpl = """
; Alias record
{asrv}            IN      CNAME   {bsrv}
"""
    with open(os.path.join(odir, f"forward.{zone}"), "w") as frwd:
        frwd.write(base_tmpl)
        for ip in ips:
            sip = straight_ip(ip)
            frwd.write(a_rec_tmpl.format(srv=srvs[0], ip=sip))
        for s in srvs[1:]:
             frwd.write(cname_rec_tmpl.format(asrv=s, bsrv=srvs[0]))

def gen_reverse_conf(zone: str, ips: List[str], srvs: List[str], odir: str):
    base_tmpl = f'''$TTL 1s

@               IN      SOA     dns1.{zone}.    hostmaster.{zone}. (
                1        ; serial
                1m       ; refresh after
                12s      ; retry after
                1w       ; expire after
                5s )     ; minimum TTL
;
;
;Name Server Information
@               IN      NS      dns1.{zone}.
dns1            IN      A       127.0.0.1

;Reverse IP Information
;Configuring IP addresses should be added here
;Each IP should be in reverse order
'''
    ptr_rec_tmpl = "{revip}.in-addr.arpa.      IN      PTR       {srv}.{zone}.\n"

    with open(os.path.join(odir, f"reverse.{zone}"), "w") as rvs:
        rvs.write(base_tmpl)
        for ip in ips:
            revip = reverse_ip(ip)
            rvs.write(ptr_rec_tmpl.format(revip=revip, srv=srvs[0], zone=zone))

if __name__ == "__main__":
    args=parse_args()

    gen_named_conf(args.zone, args.ip, args.outdir)
    gen_forward_conf(args.zone, args.ip, args.services, args.outdir)
    gen_reverse_conf(args.zone, args.ip, args.services, args.outdir)
