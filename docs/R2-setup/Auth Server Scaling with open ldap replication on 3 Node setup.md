## Auth Server Scaling with open ldap replication on 3 Node setup <h1> 


1. After provisioning 3 VMs,  configure ldap replication as explained above on each node. 
 
   `Openldap_Replication_on_3_node_setup` :- 

2. **Prerequisite** -  openldap is installed and configured correctly. 

   **To ensure this run below on all nodes** – 

    `ldapsearch -b "dc=s3,dc=seagate,dc=com" -x -w seagate -D "cn=admin,dc=seagate,dc=com"`

    This should return successfully without any errors. 

3. **Setup Replication** - 

   -  Consider you have 3 nodes. Say node 1, node 2. and node 3. and you have to setup replication among these three nodes. 
     
    **Note**: Below 4 steps you have to perform on all three nodes with one change in olcseverid.ldif that is `olcseverrid  = 1 for node 1` ,`olcserverId =2 for node 2`                       ,`olcserverId = 3 for node 3`. rest all steps will be same. 

   > Also update the hostname in provider field in `config.ldif` on all 3 nodes if not updated before running command. 

```
All the commands should run successfully. Observe the results. There should not be any error statement like – 

1. Invalid syntax 
2. No such attribute 
```
 

**There are few ldif files that you have to push to ldap in same order below (On NODE 1)-**

```
 cd /opt/seagate/cortx/s3/install/ldap/replication
```

 - All the below files will be present in this directory. You will not be required to copy the contents of the files from this page. The file contents shown below are just for reference. Edit only the relevant fields as mentioned above (for files - `olcserverid.ldif` & `config.ldif`). 

**NOTE**: Contents from below files are indentation sensitive. DO NOT alter any indentation / space / line breaks!!! 


     1. You have to push unique olcserver Id  

         olcserverid.ldif

          dn: cn=config 

         changetype: modify 

         add: olcServerID 

         olcServerID: 1 

       command to add : ldapmodify -Y EXTERNAL -H ldapi:/// -f olcserverid.ldif 

     2. loading provider module 

          syncprov_mod.ldif 

           dn: cn=module,cn=config 

          objectClass: olcModuleList 

          cn: module 

          olcModulePath: /usr/lib64/openldap 

          olcModuleLoad: syncprov.la 

          command to add - ldapadd -Y EXTERNAL -H ldapi:/// -f syncprov_mod.ldif 

     3.Push Provider ldif for config replication 

          syncprov_config.ldif 

           dn: olcOverlay=syncprov,olcDatabase={0}config,cn=config 

           objectClass: olcOverlayConfig 

           objectClass: olcSyncProvConfig 

           olcOverlay: syncprov 

           olcSpSessionLog: 100 

          command to add - ldapadd -Y EXTERNAL -H ldapi:/// -f  syncprov_config.ldif 

     4.Push Config.ldif 

         config.ldif  

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

`command to add - ldapmodify -Y EXTERNAL  -H ldapi:/// -f config.ldif` 

  

* The following 2 steps need to be performed ONLY ON ONE NODE. In our case we will perform it on `PRIMARY NODE` (node with olcserverId=1) 

* You need not Push these 2 steps on node 2 and node 3, because you have already pushed config replication on all three nodes, so adding this data.ldif on one node will replicate on all other nodes. 

 1.push provider for data replication 

    syncprov.ldif 

      dn: olcOverlay=syncprov,olcDatabase={2}mdb,cn=config 

      objectClass: olcOverlayConfig 

      objectClass: olcSyncProvConfig 

      olcOverlay: syncprov 

      olcSpSessionLog: 100 

      command to add - ldapadd -Y EXTERNAL -H ldapi:/// -f  syncprov.ldif 


   2.push data replication ldif 

Update the hostname in provider field in data.ldif on Node 1 if not updated before running command 

    data.ldif 

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

 

- command to add - `ldapmodify -Y EXTERNAL -H ldapi:/// -f data.ldif` 


```
  Finally perform below on all 3 nodes. 
```
- systemctl restart slapd 

- systemctl restart s3authserver 

 

 

 

 

 

 

 
