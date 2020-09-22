# 3 Node Manual S3 Cluster Setup

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

## Steps to configure 3 node cluster manually

1. **Order 3 fresh ssc vms from** -- https://ssc-cloud.colo.seagate.com/ui/service/catalogs

2. **Perform manual HARE / Motr cluster setup steps mentioned at below location** - 

   https://seagatetechnology.sharepoint.com/:w:/s/gteamdrv1/tdrive1224/EfvqJjlha8pNkOeIfCgDywUBn0YNIYGT6-tWMREx9iGxpw?e=D9OLR8 

   Confirm the installation is complete and S3 & mero services are up and running - `hctl status`

3. **OpenLdap Installation** - 

   Go to `/opt/seagate/cortx/s3/install/ldap` location and run `setup_ldap.sh` as below- 

   ```
   ./setup_ldap.sh --defaultpasswd --skipssl --forceclean 
   ```

   Once LDAP is set up on all 3 nodes, perform ldap replication changes using below doc - 
   
   [Auth Server Scaling with open ldap replication on 3 Node setup.md](Auth%20Server%20Scaling%20with%20open%20ldap%20replication%20on%203%20Node%20setup.md)

   Kindly skip provisioned vms setup steps and perform ldap replication part only. 

4. **Configure slapd.log on all 3 nodes** - 

   ```
   cp /opt/seagate/cortx/s3/install/ldap/rsyslog.d/slapdlog.conf /etc/rsyslog.d/slapdlog.conf 
   systemctl restart slapd 
   systemctl restart rsyslog
   ```
 
5. **HAProxy Installation on all nodes** - 

   1. Run the following command to install HAProxy - `yum install haproxy`

   2. Copy the file – `s3server.pem` from location - https://github.com/Seagate/cortx-s3server/tree/dev/ansible/files/certs/stx-s3/s3 and paste it to directory - `/etc/ssl/stx-s3/s3/`

   3. Navigate to `/opt/seagate/cortx/s3/install/haproxy`.

   4. Copy entire contents of `haproxy_osver7.cfg` (or `haproxy_osver8.cfg` depending on your OS version) to `/etc/haproxy/haproxy.cfg`
 
6. **HAProxy scaling** - 
 
   For scaling on 3 node, follow the instructions from guide - 
 
   [Haproxy 3 Node load balancing.md](Haproxy%203%20Node%20load%20balancing.md).
   
   Start haproxy – `systemctl start haproxy`
    
7. **Start authserver by following command**

   ```
   systemctl start s3authserver 
   ```

8. **Update** `/etc/hosts` file on all server nodes and append below entries corresponding to the IP address 127.0.0.1 - 
    `s3.seagate.com sts.seagate.com iam.seagate.com sts.cloud.seagate.com`


## Setting up the client node

1. **Update** `/etc/hosts` file and append below entries corresponding to the IP address of the Master node (Active HAProxy node) - 
     `s3.seagate.com sts.seagate.com iam.seagate.com sts.cloud.seagate.com `

 
2. **Download and copy stx-s3-clients from**
    ```
    http://gerrit.mero.colo.seagate.com/plugins/gitiles/s3server/+/master/ansible/files/certs/ 
    ```
    to `/etc/ssl` (you can use [tgz] option then untar on the node and move to proper places). 

3. **Install s3iamcli** - 
    
   Create `/etc/yum.repos.d/epel.repo` and add below content - 
    
   ```
   [epel] 
   gpgcheck=0 
   enabled=1 
   baseurl= http://ssc-satellite1.colo.seagate.com/pulp/repos/EOS/Production/CentOS-7_7_1908/custom/EPEL-7/EPEL-7/ 
   name=Yum repo for epel7 
   ``` 

4. **Perform below Steps**- 

   `yum install -y http://cortx-storage.colo.seagate.com/releases/eos/uploads/rhel/rhel-7.7.1908/s3server_uploads/python36-jmespath-0.9.0-1.el7.noarch.rpm --nogpgcheck`

   `yum install -y http://cortx-storage.colo.seagate.com/releases/eos/github/release/rhel-7.7.1908/last_successful/cortx-s3iamcli-1.0.0-1001_git7c59eae.noarch.rpm --nogpgcheck` 

 
5. **Install aws cli** 

   1. **Install**
      ```
       sudo easy_install pip # (or get pip installed through any other method) 
       pip install awscli 
       pip install awscli-plugin-endpoint 
      ```

   2. **Edit config using**
      ```
      aws configure 
      ```

   3. **Set Endpoint using** 
      ```
      aws configure set plugins.endpoint awscli_plugin_endpoint 
      aws configure set s3.endpoint_url http://s3.seagate.com 
      aws configure set s3api.endpoint_url http://s3.seagate.com 
      ```
  
   4. **For SSL Certificate** – add a line to `~/.aws/config` file above - `[plugins]` section 
      ```
       ca_bundle = /etc/stx-s3-clients/ca.crt
      ```

   5. The ultimate `~/.aws/config` would look something like - 

      **cat ~/.aws/config**  
      ```
      [default] 
      region = US 
      s3 =
          endpoint_url = https://s3.seagate.com 
      s3api =
          endpoint_url = https://s3.seagate.coma 
      ca_bundle = /etc/stx-s3-clients/ca.crt 
      [plugins] 
      endpoint = awscli_plugin_endpoint 
      ```

   6. Self-check: run command to confirm setup is complete
      ```
      aws s3 ls
      ```
   
      This will not return anything and should not return any error. 

6. **S3Bench Install Configure**
    
   Follow the guide to install and run S3bench - 
    
   <https://seagatetechnology.sharepoint.com/:w:/r/sites/gteamdrv1/tdrive1224/_layouts/15/Doc.aspx?sourcedoc=%7B8F1347B8-DD98-4D0D-9D57-2B2D3D48D135%7D&file=S3bench%20setup.docx&action=default&mobileredirect=true&cid=2e12a38a-ff01-459c-8eaf-2b89ebdc4572>


## References for restarting services

### Shutdown the services

1. Execute below on master -  
   ```
   hctl shutdown (This will bring down mero and s3server on all the nodes in cluster) 
   ```
2. Execute below on all the nodes to shutdown authserver 
   ```
   systemctl stop s3authserver 
   ```
3. Execute below on all the nodes to shutdown openldap 
   ```
   systemctl stop slapd 
   ```
4. Execute below on all the nodes to shutdown haproxy  
   ```
   systemctl stop haproxy 
   ```

### Re-Starting the services

1. Execute below on master -  
   ```
   hctl bootstrap --mkfs $HOME/threenodes.yaml 
   This will start s3server and motr on all the nodes of cluster 
   ```
2. Execute below on all the nodes to start Auth server- 
   ```
   systemctl start s3authserver 
   ```
3. Execute below on all the nodes to start openldap - 
   ```
   systemctl start slapd 
   ```
4. Execute below on all the nodes to start haproxy- 
   ```
   systemctl start haproxy 
   ```
