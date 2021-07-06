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


#####################################
# Script to configure dns round robin
#####################################

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
        description="""Configure BIND dns server to round robin IPs.""",
        epilog=f"""WARNING:
If zone is configured as 'seagate.com' it will hide global definition of 'seagate.com' services.
If it is required to have an access to global 'seagate.com' local dns server could be temporary
disabled and enabled later.

Examples:
To configure with default params: zone seagate.com and services s3, iam
        #> {sys.argv[0]} install --ip 192.168.1.0 192.168.1.1

To configure for different zones and services
        #> {sys.argv[0]} install --ip 192.168.1.0 192.168.1.1 --zone test.zone --services srv1 srv2

To temporary disable local dns server
        #> {sys.argv[0]} disable

To enable it again
        #> {sys.argv[0]} enable
        """)

    argz = parser.add_argument("-z", "--zone", type=str, required=False, default="seagate.com",
                               help="Name of the zone to create")
    argsrvs = parser.add_argument("-s", "--services", nargs="+", type=str, default=["s3", "iam"],
                                  required=False,
                                  help="Services that should be created inside zone and mapped to IPs")
    argip = parser.add_argument("-i", "--ip", nargs="+", type=str, default=[], required=False,
                                help="IP addresses")
    parser.add_argument("operation", choices=['install', 'disable', 'enable'], default="install",
                        help="Operation to perform")
    args = parser.parse_args()

    if args.operation != "install":
        return args

    if not args.ip:
        raise argparse.ArgumentError(argip, "At least one IP should be provided")

    for ip in args.ip:
        if not chk_ip(ip):
            raise argparse.ArgumentError(argip, "Invalid IP adress format")

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

def upd_resolv_conf():
    cont = []
    with open("/etc/resolv.conf", "r") as rc:
        cont = rc.readlines()

    with open("/etc/resolv.conf", "w") as rc:
        i = 0
        while i < len(cont):
            if cont[i].startswith("nameserver"):
                break
            rc.write(cont[i])
            i += 1
        rc.write("nameserver 127.0.0.1\n")
        rc.writelines(cont[i:])

def ns_toggle_resolv_conf(ensure_commented: bool):
    cont = []
    with open("/etc/resolv.conf", "r") as rc:
        cont = rc.readlines()

    for i, v in enumerate(cont):
        if "127.0.0.1" in v:
            if ensure_commented:
                cont[i] = "#nameserver 127.0.0.1\n"
            else:
                cont[i] = "nameserver 127.0.0.1\n"

    with open("/etc/resolv.conf", "w") as rc:
        rc.writelines(cont)

if __name__ == "__main__":
    args=parse_args()

    if args.operation == "install":
        print("Install BIND...\n")

        ret = os.system("yum install -y bind bind-utils")
        print(f"Done with ret code {ret}\n")
        if ret != 0:
            exit(ret)

        print("Generate configs...\n")
        gen_named_conf(args.zone, args.ip, "/etc")
        gen_forward_conf(args.zone, args.ip, args.services, "/var/named")
        gen_reverse_conf(args.zone, args.ip, args.services, "/var/named")
        print(f"Created\n/etc/named.conf\n/var/named/forward.{args.zone}\n/var/named/reverse.{args.zone}\n")

        print("Restart BIND...\n")
        os.system("systemctl enable named.service")
        os.system("systemctl restart named.service")
        ret = os.system("systemctl status named.service")
        print(f"Done with ret code {ret}\n")
        if ret != 0:
            exit(ret)

        print("Update resolve.conf...\n")
        upd_resolv_conf()
        print(f"Done\n")

        print("DNS Round Robin configuraion finished\n")
    elif args.operation == "disable":
        print("Disabling DNS Round Roubin")

        print("Stoping BIND...\n")
        os.system("systemctl disable named.service")
        ret = os.system("systemctl stop named.service")
        print(f"Done with ret code {ret}\n")

        print("Commenting out local nameserver in resolve.conf...\n")
        ns_toggle_resolv_conf(True)
        print("Done\n")

        print("DNS Roun Roubin disabled\n")
    elif args.operation == "enable":
        print("Enabling DNS Roun Roubin configuration")

        print("Starting BIND...\n")
        os.system("systemctl enable named.service")
        ret = os.system("systemctl start named.service")
        print(f"Done with ret code {ret}\n")

        print("Enabling local nameserver in resolve.conf...\n")
        ns_toggle_resolv_conf(False)
        print("Done\n")

        print("DNS Roun Roubin enabled\n")
