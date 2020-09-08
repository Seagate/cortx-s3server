# DNS Round Robin Configuration

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

This document describes how to setup and configure local DNS server.

Several IP addresses will be configured to match the same domain name.
DNS server will return these IP addresses in round robin manner.

*BIND* (<https://www.isc.org/bind/>) is used as a DNS server.
Suppose we have 3 nodes each has 2 IPs

+ *Node 1*
  + 192.168.1.10
  + 192.168.1.11

+ *Node 2*
  + 192.168.2.10
  + 192.168.2.11

+ *Node 3*
  + 192.168.3.10
  + 192.168.3.11

It is needed to map all these IPs to domain name *s3.seagate.com*

Domain name *iam.seagate.com* should also be mapped to the same addresses

## How to choose IP adresses

If DNS server returns multiple addresses it is up to client to deside which
address to use.
According to *[RFC3484](https://www.ietf.org/rfc/rfc3484.txt)* if client and
requested addresses belong to the same network client should take the longest
matching address negating the effects of DNS server round robin. For example
assume that client address is *192.168.0.1* and DNS server return the list of
addresses

+ *192.168.1.10*
+ *192.168.5.20*
+ *192.168.6.30*
+ *192.168.0.40*
+ *192.168.4.50*

Due to *RFC3484* client will always choose *192.168.0.40* as the longet match
with no respect the order addresses returned by DNS server.

There are two possible ways to avoid client address reordering

+ addresses returned by DNS server should be consecutive and continuous
so the longest match would be the same for all IPs in the list, e.g.

  + 192.168.1.1
  + 192.168.1.2
  + 192.168.1.3
  + etc.

+ client should belong to the different subnetwork returned IP addresses belong
to. In this case it is possible that returned IP addresses belong to different
subnetworks, but all this subnetworks should be different from client's
subnetwork, e.g.

  + Addresses returned by the DNS server
    + 192.168.1.1/24
    + 192.168.50.1/26
    + 192.168.150.1/20
    + 192.168.250.1/28

  + Client address
    + 10.0.0.1/20

## Install

This command installs *BIND*

`#> yum install bind bind-utils`

## Configuration

*BIND’s* configuration consists of several files

+ */etc/named.conf* - main configuration file
+ forward zone file - included from */etc/named.conf* file
+ reverse zone file - included from */etc/named.conf* file

## Main configuration file

***/etc/named.conf***

```dns
options {
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

// This should be added to provide Forward zone description
zone "seagate.com"  {
        type master;
        file    "/var/named/forward.seagate.com";
};

// This should be added to provide Reverse zone description
zone   "168.192.in-addr.arpa"  {
       type master;
       file    "/var/named/reverse.seagate.com";
};
```

## Forward zone file

***/var/named/forward.seagate.com***

```dns
$TTL 1s

@               IN      SOA     dns1.seagate.com.    hostmaster.seagate.com. (
                1        ; serial
                1m       ; refresh after
                12s      ; retry after
                1w       ; expire after
                5s )     ; minimum TTL
;
;
;Name Server Information
@               IN      NS      dns1.seagate.com.
dns1             IN      A      127.0.0.1

; Alias record for IAM that should be mapped to the same addresses as s3
iam            IN      CNAME   s3
;
; A Records:
; Configuring IP addresses should added here
s3             IN      A       192.168.1.10
s3             IN      A       192.168.2.10
s3             IN      A       192.168.3.10
s3             IN      A       192.168.1.11
s3             IN      A       192.168.2.11
s3             IN      A       192.168.3.11
```

This file should contain IP addresses of the server nodes. Configuring IP
addresses should be listed in a proper order. In our example each node has 2
IP addresses. To get a fair load distribution accross the nodes - new
reqiest goes to the new node - the listed addresses should be in a following
order: first address from the first node, first address from the second node,
first address from the third node, second address from the first node, second
address from the second node, second address from the third node.

## Reverse zone file

***/var/named/reverse.seagate.com***

```dns
$TTL 10s

@               IN      SOA     seagate.com.    hostmaster.seagate.com. (
                1        ; serial
                1m       ; refresh after
                12s      ; retry after
                1w       ; expire after
                5s )     ; minimum TTL
;
;
;Name Server Information
@               IN      NS      dns1.seagate.com.
dns1             IN      A      127.0.0.1
;
;
;Reverse IP Information
;Configuring IP addresses should be added here
;Each IP should be in reverse order
10.1.168.192.in-addr.arpa.      IN      PTR       s3.seagate.com.
10.2.168.192.in-addr.arpa.      IN      PTR       s3.seagate.com.
10.3.168.192.in-addr.arpa.      IN      PTR       s3.seagate.com.
11.1.168.192.in-addr.arpa.      IN      PTR       s3.seagate.com.
11.2.168.192.in-addr.arpa.      IN      PTR       s3.seagate.com.
11.3.168.192.in-addr.arpa.      IN      PTR       s3.seagate.com.
```

This file should contain IP addresses of the server nodes. Each address should
be written in reverse order.

## Test configuration

Changes to configuration files could be tested with the following commands.
If configuration is valid then no errors will be shown

`#> named-checkconf /etc/named.conf`  
`#> named-checkzone seagate.com /var/named/forward.seagate.com`  
`#> named-checkzone seagate.com /var/named/reverse.seagate.com`

## Run DNS server

`#> systemctl enable named.service`  
`#> systemctl start named.service`

## Test server

Command reports status of the service. Service should be running, service log
should not contain error messages

`#> systemctl status named.service`

Command performs DNS lookups and displays the responses returned.
The output should contain addresses from the configuration

`#> dig @127.0.0.1 -t A s3.seagate.com`  
`#> dig @127.0.0.1 -t A iam.seagate.com`

## Configure client

To be able to use *s3.seagate.com* domain name just created DNS server
should be added as a first server to the list of servers in client's
***/etc/resolv.conf*** file.

***/etc/resolv.conf***

```bash
nameserver 127.0.0.1

```

## Test client

Several consecutive *nslookup* commands could be used to verify different IP
addresses are returned

`nslookup s3.seagate.com`  
`nslookup s3.seagate.com`  
`nslookup s3.seagate.com`

Following command will test *iam.seagate.com* domain name

`nslookup iam.seagate.com`
