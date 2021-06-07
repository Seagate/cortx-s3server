# HAProxy with Load balancing using DNS RR and Keepalived

### License

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

## HAProxy configuration manual steps

1. Before configuring HAProxy, check the number of S3 instances from hctl status. 

   ```
    [root@ssc-vm-1040 ~]# hctl status
    Profile: 0x7000000000000001:0x72
    Data pools:
        0x6f00000000000001:0x73
    Services:
    ssc-vm-1040.colo.seagate.com
    [started]  hax        0x7200000000000001:0x6   10.230.247.96@tcp:12345:1:1
    [started]  confd      0x7200000000000001:0x9   10.230.247.96@tcp:12345:2:1
    [started]  ioservice  0x7200000000000001:0xc   10.230.247.96@tcp:12345:2:2
    [started]  s3server   0x7200000000000001:0x1c  10.230.247.96@tcp:12345:3:1
    [started]  s3server   0x7200000000000001:0x1f  10.230.247.96@tcp:12345:3:2
    [unknown]  m0_client  0x7200000000000001:0x22  10.230.247.96@tcp:12345:4:1
    [unknown]  m0_client  0x7200000000000001:0x25  10.230.247.96@tcp:12345:4:2
    ssc-vm-1041.colo.seagate.com
    [started]  hax        0x7200000000000001:0x2b  10.230.247.127@tcp:12345:1:1
    [started]  confd      0x7200000000000001:0x2e  10.230.247.127@tcp:12345:2:1
    [started]  ioservice  0x7200000000000001:0x31  10.230.247.127@tcp:12345:2:2
    [started]  s3server   0x7200000000000001:0x41  10.230.247.127@tcp:12345:3:1
    [started]  s3server   0x7200000000000001:0x44  10.230.247.127@tcp:12345:3:2
    [unknown]  m0_client  0x7200000000000001:0x47  10.230.247.127@tcp:12345:4:1
    [unknown]  m0_client  0x7200000000000001:0x4a  10.230.247.127@tcp:12345:4:2
    ssc-vm-1099.colo.seagate.com  (RC)
    [started]  hax        0x7200000000000001:0x50  10.230.242.144@tcp:12345:1:1
    [started]  confd      0x7200000000000001:0x53  10.230.242.144@tcp:12345:2:1
    [started]  ioservice  0x7200000000000001:0x56  10.230.242.144@tcp:12345:2:2
    [started]  s3server   0x7200000000000001:0x66  10.230.242.144@tcp:12345:3:1
    [started]  s3server   0x7200000000000001:0x69  10.230.242.144@tcp:12345:3:2
    [unknown]  m0_client  0x7200000000000001:0x6c  10.230.242.144@tcp:12345:4:1
    [unknown]  m0_client  0x7200000000000001:0x6f  10.230.242.144@tcp:12345:4:2
    [root@ssc-vm-1040 ~]#
    ```

2. Lets say we have 2 s3server instances each node . Hence each node HAProxy will be configured with 2 s3 instances in the HAProxy’s  `backend` section of app-main.  

3. Open `/etc/haproxy/haproxy.cfg` and navigate to `backend app-main` section. 

4. Locate S3 instance - `server s3-instance-1 0.0.0.0:28071 check maxconn 110`. Replace the 0.0.0.0 of all instances with the IP addresses (eth0) of the current node 

5. Keep the instance name `s3-instance-x` for each instance unique, incrementing `x` by 1. 

6. Also increment the port number `28071` for the next instances by 1. 

7. Navigate to `backend s3-auth` section 

8. Locate S3auth instance - `server s3authserver-instance1 0.0.0.0:9085`. Replace 0.0.0.0 with the IP address of current node - (This is not necessary but required for maintainability during scale out). 

9. Comment out if present – the entire section of `HAProxy Monitoring Config`

10. Edit `/etc/haproxy/haproxy.cfg` on all the cluster  nodes as above using the respective nodes IP . 

11. Final HAProxy file will look like this :  

   ```
   [root@ssc-vm-0918 ~]# cat /etc/haproxy/haproxy.cfg 
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

   #--------------------------------------------------------------------- 
   # Example configuration for a possible web application.  See the 
   # full configuration options online. 
   # 
   #   http://haproxy.1wt.eu/download/1.4/doc/configuration.txt 
   # 
   #--------------------------------------------------------------------- 

   # haproxy --v command will show current/build settings 

   #---------------------------------------------------------------------
   # Global settings 
   #--------------------------------------------------------------------- 
   global 
       log         127.0.0.1 local2 info    #Log configuration 
       # create a rsyslog.d/haproxy.conf with rules to create 
       # /var/log/haproxy.log file. 

       chroot      /var/lib/haproxy 
       pidfile     /var/run/haproxy.pid 
       maxconn     4000 
       user        haproxy             #Haproxy running under user and group "haproxy" 
       group       haproxy 
       # nbproc value need to be set to 8 in physical systems. 
       nbproc      2 
       daemon   

       # turn on stats unix socket 
       stats socket /var/lib/haproxy/stats 

       #SSL options    
       tune.ssl.default-dh-param 2048 

   #--------------------------------------------------------------------- 
   # common defaults that all the 'listen' and 'backend' sections will 
   # use if not designated in their block 
   #--------------------------------------------------------------------- 
   defaults 
       mode                    http 
       option                  redispatch 
       log                     global 
       option                  httplog 
       option                  log-separate-errors 
       option                  dontlognull 
       option                  http-tunnel 
       option                  forwardfor 
       errorfile               503 /etc/haproxy/errors/503.http 

       retries                 3 

       timeout http-request    10s 
       timeout queue           1m 

       # Connect timeout to server 
       timeout connect         5s 

       # Inactivity timeout w.r.t S3 client 
       timeout client          30s 

       # Inactivity timeout w.r.t backend S3 servers 
       timeout server          30s 

       timeout http-keep-alive 10s 
       timeout tunnel          60s 
       timeout client-fin      20s 
       timeout check           10s 
       maxconn                 3000 

   #--------------------------------------------------------------------- 
   #HAProxy Monitoring Config 
   #--------------------------------------------------------------------- 
   #listen haproxy3-monitoring *:8080                #Haproxy Monitoring run on port 8080 
   #    mode http 
   #    option forwardfor 
   #    option httpclose 
   #    stats enable 
   #    stats show-legends 
   #    stats refresh 5s 
   #    stats uri /stats                             #URL for HAProxy monitoring 
   #    stats realm Haproxy\ Statistics 
       #stats auth howtoforge:howtoforge            #User and Password for login to the monitoring dashboard 
       #stats admin if TRUE 
       #default_backend app-main                    #This is optionally for monitoring backend 

   #--------------------------------------------------------------------- 
   # FrontEnd Configuration 
   #--------------------------------------------------------------------- 
   frontend main 
       # ssl-passthrough reference: 
       # https://serversforhackers.com/c/using-ssl-certificates-with-haproxy 


       # s3 server port 
       bind 0.0.0.0:80 
       bind 0.0.0.0:443 ssl crt /etc/ssl/stx-s3/s3/s3server.pem 
       option forwardfor 
       default_backend app-main 

       # s3 auth server port 
       bind 0.0.0.0:9080 
       bind 0.0.0.0:9443 ssl crt /etc/ssl/stx-s3/s3/s3server.pem 

       acl s3authbackendacl dst_port 9443 
       acl s3authbackendacl dst_port 9080 
       use_backend s3-auth if s3authbackendacl 

   #--------------------------------------------------------------------- 
   # BackEnd roundrobin as balance algorithm 
   #--------------------------------------------------------------------- 
   backend app-main 
       balance static-rr                                     #Balance algorithm 
       http-response set-header Server SeagateS3 
       # Check the S3 server application is up and healthy - 200 status code 
       option httpchk HEAD / HTTP/1.1\r\nHost:\ localhost 

       # option log-health-checks 
       default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter 

       # For ssl communication between haproxy and s3server 
       # Replace below line 
       server s3-instance-1 10.230.243.16:28071 check maxconn 110        # s3 instance 1 
       server s3-instance-2 10.230.243.16:28072 check maxconn 110        # s3 instance 2 
       # with 
       # server s3-instance-1 0.0.0.0:28071 check maxconn 110 ssl verify required ca-file /etc/ssl/stx-s3/s3/ca.crt 

       # server s3-instance-2 0.0.0.0:28072 check maxconn 110      # s3 instance 2 

   #---------------------------------------------------------------------- 
   # BackEnd roundrobin as balance algorith for s3 auth server 
   #---------------------------------------------------------------------- 
   backend s3-auth 
       balance static-rr                                     #Balance algorithm 

       # Check the S3 Auth server application is up and healthy - 200 status code 
       option httpchk HEAD /auth/health HTTP/1.1\r\nHost:\ localhost 

       # option log-health-checks 
       default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter 

       server s3authserver-instance1 10.230.243.16:9085 #check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance 1 
       # server s3authserver-2 s3auth-node2:9086 check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance 2 
       # server s3authserver-3 s3auth-node3:9086 check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance 3 
       # server s3authserver-4 s3auth-node4:9086 check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance 4 

       #server s3authserver 0.0.0.0:9085 check             # s3 auth server No SSL
   ```

12. Configure haproxy logs on all the nodes - 

    ```
    mkdir /etc/haproxy/errors/ 
    cp /opt/seagate/cortx/s3/install/haproxy/503.http /etc/haproxy/errors/ 
    cp /opt/seagate/cortx/s3/install/haproxy/logrotate/haproxy /etc/logrotate.d/haproxy 
    cp /opt/seagate/cortx/s3/install/haproxy/rsyslog.d/haproxy.conf /etc/rsyslog.d/haproxy.conf 
    rm -rf /etc/cron.daily/logrotate 
    cp /opt/seagate/cortx/s3/install/haproxy/logrotate/logrotate /etc/cron.hourly/logrotate 
    systemctl restart rsyslog 
    systemctl restart haproxy 
    systemctl status haproxy         ## (HAProxy should be running and active) 
    ```
