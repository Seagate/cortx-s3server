# Auth Server Scaling with open ldap replication on 3 Node setup

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


## Manual Steps to set up Auth Server

### Prerequisites

OpenLDAP has to be installed and configured correctly.  (Should have been taken care of when doing initial node deployment with provisioner.)

**To ensure this run below on all nodes** – 

```
ldapsearch -b "dc=s3,dc=seagate,dc=com" -x -w seagate -D "cn=admin,dc=seagate,dc=com"
```

This should return successfully without any errors. 


### Setup Replication

Consider you have 3 nodes. Say node 1, node 2. and node 3. and you have to setup replication among these three nodes. 

**Note**: Below 4 steps you have to perform on all three nodes with one change in olcseverid.ldif that is:

* `olcSeverrId: 1` for node 1,
* `olcServerId: 2` for node 2,
* `olcserverId: 3` for node 3.

Rest all steps will be same. 

**Note 2**: Make sure to update the hostname in `provider` field in `config.ldif` on all 3 nodes if not updated before running command. 

**Note 3**: All the commands should run successfully. Observe the results. There should not be any error statement like – 

* `Invalid syntax`
* `No such attribute`


#### Steps to be run on all nodes in cluster

Steps:

1. Open the console on the node and do:

   ```
   cd /opt/seagate/cortx/s3/install/ldap/replication
   ```

   All the files referenced will be present in this directory. You will not be required to copy the contents of the files from this page -- you can copy and edit those files in `replication` folder. The file contents shown below are just for reference. Edit only the relevant fields as mentioned above (for files - `olcserverid.ldif` and `config.ldif`). 

   **NOTE**: Contents in these files is indentation sensitive. DO NOT alter any indentation / space / line breaks!!! 

2. Push unique olcServerId:

   `olcserverid.ldif`:

   ```
   dn: cn=config
   changetype: modify
   add: olcServerID
   olcServerID: 1
   ```
   
   Edit `olsServerID: N` where `N` is the number of current node.
   
   Command to apply:
   
   ```
   ldapmodify -Y EXTERNAL -H ldapi:/// -f olcserverid.ldif
   ```

3. Loading provider module 

   `syncprov_mod.ldif`:

   ```
   dn: cn=module,cn=config 
   objectClass: olcModuleList 
   cn: module 
   olcModulePath: /usr/lib64/openldap 
   olcModuleLoad: syncprov.la 
   ```

   Command to apply:
   
   ```
   ldapadd -Y EXTERNAL -H ldapi:/// -f syncprov_mod.ldif
   ```

4. Push Provider ldif for config replication 

   `syncprov_config.ldif`:
   
   ```
   dn: olcOverlay=syncprov,olcDatabase={0}config,cn=config 
   objectClass: olcOverlayConfig 
   objectClass: olcSyncProvConfig 
   olcOverlay: syncprov 
   olcSpSessionLog: 100 
   ```
   
   Command to apply:
   
   ```
   ldapadd -Y EXTERNAL -H ldapi:/// -f  syncprov_config.ldif
   ```

5. Push Config.ldif 

   `config.ldif`:
   
   ```
    dn: olcDatabase={0}config,cn=config 
    changetype: modify 
    add: olcSyncRepl 
    olcSyncRepl: rid=001 
        provider=ldap://<hostname_node-1>:389/ 
        bindmethod=simple  
        binddn="cn=admin,cn=config" 
        credentials=seagate 
        searchbase="cn=config" 
        scope=sub 
        schemachecking=on 
        type=refreshAndPersist 
        retry="30 5 300 3" 
        interval=00:00:05:00 
    # Enable additional providers 
    olcSyncRepl: rid=002 
      provider=ldap://<hostname_node-2>:389/ 
       bindmethod=simple 
       binddn="cn=admin,cn=config" 
       credentials=seagate 
       searchbase="cn=config" 
       scope=sub 
       schemachecking=on 
       type=refreshAndPersist 
       retry="30 5 300 3" 
       interval=00:00:05:00 
    olcSyncRepl: rid=003 
       provider=ldap://<hostname_node-3>:389/ 
       bindmethod=simple 
       binddn="cn=admin,cn=config" 
       credentials=seagate 
       searchbase="cn=config" 
       scope=sub 
       schemachecking=on 
       type=refreshAndPersist 
       retry="30 5 300 3" 
       interval=00:00:05:00 
      -
       add: olcMirrorMode 
       olcMirrorMode: TRUE 
   ```
   
   **Note**: Make sure to update `hostname_node-N` with hostnames of your nodes in the file before applying.
   
   Command to apply:
   
   ```
   ldapmodify -Y EXTERNAL  -H ldapi:/// -f config.ldif
   ```

6. Repeat the above steps on every node in cluster.


#### Steps to be done on primary node

The following steps need to be performed ONLY ON ONE NODE. In our case we will perform it on `PRIMARY NODE` (node with `olcserverId=1`).

1. Update provider for data replication 

   `syncprov.ldif`:

   ```
   dn: olcOverlay=syncprov,olcDatabase={2}mdb,cn=config 
   objectClass: olcOverlayConfig 
   objectClass: olcSyncProvConfig 
   olcOverlay: syncprov 
   olcSpSessionLog: 100 
   ```
      
   Command to apply:
   
   ```
   ldapadd -Y EXTERNAL -H ldapi:/// -f  syncprov.ldif
   ```

2. Push data replication ldif 

   **Note**: Make sure to update the hostname in `provider` field in `data.ldif`.
    
   `data.ldif`:

   ```
   dn: olcDatabase={2}mdb,cn=config 
   changetype: modify 
   add: olcSyncRepl 
   olcSyncRepl: rid=004 
       provider=ldap://< hostname_of_node_1>:389/ 
       bindmethod=simple 
       binddn="cn=admin,dc=seagate,dc=com" 
       credentials=seagate 
       searchbase="dc=seagate,dc=com" 
       scope=sub 
       schemachecking=on 
       type=refreshAndPersist 
       retry="30 5 300 3" 
       interval=00:00:05:00 
   # Enable additional providers 
   olcSyncRepl: rid=005 
       provider=ldap://< hostname_of_node_2>:389/ 
       bindmethod=simple 
       binddn="cn=admin,dc=seagate,dc=com" 
       credentials=seagate 
       searchbase="dc=seagate,dc=com" 
       scope=sub 
       schemachecking=on 
       type=refreshAndPersist 
       retry="30 5 300 3" 
       interval=00:00:05:00 
   olcSyncRepl: rid=006 
       provider=ldap://<hostname_of_node_3>:389/ 
       bindmethod=simple 
       binddn="cn=admin,dc=seagate,dc=com" 
       credentials=seagate 
       searchbase="dc=seagate,dc=com" 
       scope=sub 
       schemachecking=on 
       type=refreshAndPersist 
       retry="30 5 300 3" 
       interval=00:00:05:00 
   - 
       add: olcMirrorMode 
       olcMirrorMode: TRUE 
   ```

   Command to apply:
   
   ```
   ldapmodify -Y EXTERNAL -H ldapi:/// -f data.ldif
   ```

#### Finalize the setup - steps to be run on all nodes

Finally perform below on all 3 nodes. 

```
systemctl restart slapd 
systemctl restart s3authserver 
```
