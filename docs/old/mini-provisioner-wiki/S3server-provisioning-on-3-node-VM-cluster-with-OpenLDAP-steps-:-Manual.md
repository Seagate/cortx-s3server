# Pre-Requisites
1. Install 3rd part packages on all the 3 nodes
    >`curl -s http://cortx-storage.colo.seagate.com/releases/cortx/third-party-deps/rpm/install-cortx-prereq.sh | bash`   
2. Add below repositories on all the 3 VMs (Generate your own [jenkins custom build](http://eos-jenkins.colo.seagate.com/job/GitHub-custom-ci-builds/job/centos-7.8/job/cortx-custom-ci/)) using 'main' branch of all components
   1. lustre (for cortx-motr)
       >`$yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/github/integration-custom-ci/centos-7.8.2003/custom-build-399/3rd_party/lustre/custom/tcp/`
   2. cortx iso
       >`$yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/github/integration-custom-ci/centos-7.8.2003/custom-build-399/cortx_iso/`
   3. 3rd party libraries
       >`$yum-config-manager --add-repo=http://cortx-storage.colo.seagate.com/releases/cortx/github/integration-custom-ci/centos-7.8.2003/custom-build-1120/3rd_party/` 
3. Install S3 Server rpm on all the 3 nodes
   >`$yum install -y --nogpgcheck cortx-s3server`
4. [Machine-ID of all 3 VMs](https://github.com/Seagate/cortx-motr/wiki/Motr-deployment-using-motr_setup-on-Threenode-VM.#create-a-machine-id---required-only-for-vm-not-on-hw---run-this-on-all-nodes)
5. FQDN of all 3 VMs
   >`$hostname`
   * uniqueness of FQDN is assumed
6. Cluster-ID
   >Note down cluster-id from file: `/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml`.
   Please edit this file (and template file in step 8) if the non-default cluster-ID is used
7. Openldap credentials. Execute below commands on any of the 3 VMs
   1. `$s3cipher generate_key --const_key 'cortx'`
   2. `$s3cipher encrypt --data "**any-string-of-your-choice as LDAP root secret key**" --key '**output of 7(i) step**'`
   3. `$s3cipher encrypt --data "**any-string-of-your-choice as SGIAM secret key**" --key '**output of 7(i) step**'`
8. Update following confstore config template files on all 3 VMs
   1. `/opt/seagate/cortx/s3/conf/s3.config.tmpl.3-node`
       1. Replace TMPL_MACHINE_ID_1 with machine-id of VM-1. Similarly update TMPL_MACHINE_ID_2 and TMPL_MACHINE_ID_3 with respective machine-ids of other 2 VMs
       2. Replace TMPL_CLUSTER_ID with cluster-ID from **step 6**
       3. Replace TMPL_HOSTNAME_1 with FQDN of VM-1. Similarly update TMPL_HOSTNAME_2 and TMPL_HOSTNAME_2 with respective FQDNs of other 2 VMs.
       4. Replace TMPL_ROOT_SECRET_KEY with output of step 7(ii)
       5. Replace TMPL_SGIAM_SECRET_KEY with output of step 7(iii)
   2. `/opt/seagate/cortx/s3/conf/s3.init.tmpl.1-node`
       1. Replace TMPL_MACHINE_ID with machine-id of the current VM.
       2. Replace TMPL_CLUSTER_ID with cluster-ID from **step 6**
       3. Replace TMPL_HOSTNAME with FQDN of current VM.
       4. Replace TMPL_ROOT_SECRET_KEY with output of step 7(ii)
       5. Replace TMPL_SGIAM_SECRET_KEY with output of step 7(iii)
   3. `/opt/seagate/cortx/s3/conf/s3.test.tmpl.1-node`
       1. Replace TMPL_SGIAM_SECRET_KEY with output of step 7(iii)
   
   * Add/Set **srvnode-1.data.public** as `Public IP` of VM-1 and **srvnode-1.data.private** as `Private IP` of VM-1 in `/etc/hosts` file of VM-1
   * Add/Set **srvnode-2.data.public** as `Public IP` of VM-2 and **srvnode-2.data.private** as `Private IP` of VM-2 in `/etc/hosts` file of VM-2
   * Add/Set **srvnode-3.data.public** as `Public IP` of VM-3 and **srvnode-3.data.private** as `Private IP` of VM-3 in `/etc/hosts` file of VM-3

* If not existing, create /etc/ssl/stx/stx.pem file and copy contents from here : [stx.pem](https://github.com/Seagate/cortx-prvsnr/blob/pre-cortx-1.0/srv/components/misc_pkgs/ssl_certs/files/stx.pem) on all the 3 nodes.
* (Optional) If you need client certificates on your client node, create /etc/ssl/stx-s3-clients/s3/ca.crt file and copy contents from here : [ca.crt](https://github.com/Seagate/cortx-prvsnr/blob/pre-cortx-1.0/srv/components/s3clients/files/ca.crt) on your client machine.
* If found, comment the lines containing "PROFILE=SYSTEM" from /etc/haproxy/haproxy.cfg file on all the 3 nodes
* Follow all the **pre-requisite** steps for [cortx-py-utils](https://github.com/Seagate/cortx-utils/wiki/cortx-py-utils-multi-node-manual-provisioning#pre-requisites) on all 3 nodes
* Install all openldap relevant packages `symas-openldap symas-openldap-servers symas-openldap-clients openldap-devel python36-ldap` on all 3 nodes
* Note that ldap logs are dumped at - /var/log/seagate/utils/openldap/OpenldapProvisioning.log
* Make sure [cortx-motr](https://github.com/Seagate/cortx-motr/wiki/Motr-deployment-using-motr_setup-on-Threenode-VM.) 'Pre-requisites' conditions are satisfied.
* For I/O, we need to perform below steps w.r.t cortx-motr mini-provisioning
    1. Create password-less login between all the nodes
    2. [Install cortx-motr and hare](https://github.com/Seagate/cortx-motr/wiki/Motr-deployment-using-motr_setup-on-Threenode-VM.#install-dependent-rpm-cortx-motr-and-cortx-hare---run-this-on-all-nodes)
    3. [Confstore config file for cortx-motr mini-provisioning](https://github.com/Seagate/cortx-motr/wiki/Motr-deployment-using-motr_setup-on-Threenode-VM.#modify-templates-with-the-below-changes-on-all-nodes)

# S3server Mini Provisioning 
* Follow below steps on all the 3 nodes
### S3:Post_Install
1. [utils_setup post_install](https://github.com/Seagate/cortx-utils/wiki/cortx-py-utils-multi-node-manual-provisioning#post-install-on-all-nodes)
2. `$/opt/seagate/cortx/utils/bin/openldap_setup post_install --config "yaml:///opt/seagate/cortx/utils/conf/openldap.post_install.tmpl"`
3. `$/opt/seagate/cortx/motr/bin/motr_setup post_install --config "yaml:///opt/seagate/cortx/motr/conf/motr.post_install.tmpl"`
4. `$/opt/seagate/cortx/s3/bin/s3_setup post_install --config "yaml:///opt/seagate/cortx/s3/conf/s3.post_install.tmpl.1-node"`
### S3:Prepare
    $/opt/seagate/cortx/utils/bin/openldap_setup prepare --config "yaml:///opt/seagate/cortx/utils/conf/openldap.prepare.tmpl"
    $/opt/seagate/cortx/motr/bin/motr_setup prepare --config "yaml:///opt/seagate/cortx/motr/conf/motr.prepare.tmpl"
    $/opt/seagate/cortx/s3/bin/s3_setup prepare --config "yaml:///opt/seagate/cortx/s3/conf/s3.prepare.tmpl.1-node"
### S3:Config
* All 3rd party services are expected to be run before config stage as per [CORTX Components Mini Provisioning Deliverables](https://seagate-systems.atlassian.net/wiki/spaces/PRIVATECOR/pages/84803862/CORTX+Components+Mini+Provisioner+Deliverables) 
* Use [s3prov_start_services.sh](https://github.com/Seagate/cortx-s3server/blob/main/scripts/s3prov_start_services.sh) to run required 3rd party services
    >`sh ./s3prov_start_services.sh haproxy slapd rsyslog sshd`
* [utils_setup config](https://github.com/Seagate/cortx-utils/wiki/cortx-py-utils-multi-node-manual-provisioning#config-on-all-nodes)
* >`$/opt/seagate/cortx/utils/bin/openldap_setup config --config "yaml:///opt/seagate/cortx/utils/conf/openldap.config.tmpl.3-node"`
* >`/opt/seagate/cortx/motr/bin/motr_setup config --config "yaml:///opt/seagate/cortx/motr/conf/motr.config.tmpl"`
* >`$/opt/seagate/cortx/s3/bin/s3_setup config --config "yaml:///opt/seagate/cortx/s3/conf/s3.config.tmpl.3-node"`
### S3:Init
    $/opt/seagate/cortx/utils/bin/openldap_setup init --config "yaml:///opt/seagate/cortx/utils/conf/openldap.init.tmpl.3-node"
    $/opt/seagate/cortx/s3/bin/s3_setup init --config "yaml:///opt/seagate/cortx/s3/conf/s3.init.tmpl.1-node"

# Start services for I/O 
1. [Hare bootstrap](https://github.com/Seagate/cortx-motr/wiki/Motr-deployment-using-motr_setup-on-Threenode-VM.#create-cdf-file)

# Optional Test and Teardown (Mini-Provisioning) steps (To be run all 3 nodes)

### S3:Test
   1. Install test RPM if not already installed
   2. $/opt/seagate/cortx/s3/bin/s3_setup test --config "yaml:///opt/seagate/cortx/s3/conf/s3.test.tmpl.1-node" --plan "test_plan"

### S3:Reset
   >$/opt/seagate/cortx/s3/bin/s3_setup reset --config "yaml:///opt/seagate/cortx/s3/conf/s3.reset.tmpl.1-node"
   >$/opt/seagate/cortx/utils/bin/openldap_setup reset --config "yaml:///opt/seagate/cortx/utils/conf/openldap.reset.tmpl"
   >$/opt/seagate/cortx/utils/bin/utils_setup reset --config yaml:///tmp/utils.reset.tmpl.1-node

### S3:Cleanup
   >$/opt/seagate/cortx/s3/bin/s3_setup cleanup --config "yaml:///opt/seagate/cortx/s3/conf/s3.cleanup.tmpl.1-node"
   >$/opt/seagate/cortx/utils/bin/openldap_setup cleanup --config "yaml:///opt/seagate/cortx/utils/conf/openldap.cleanup.tmpl"
   >$/opt/seagate/cortx/utils/bin/utils_setup cleanup --config yaml:///tmp/utils.cleanup.tmpl.1-node
   
# Optional Pre-Upgrade and Post-Upgrade steps
### Stop the cluster (On primary node).
    $hctl shutdown
### S3:Pre-Upgrade (All nodes)
    $/opt/seagate/cortx/s3/bin/s3_setup preupgrade
### S3:RPM-Upgrade (All nodes)
    $yum upgrade cortx-s3server-2.0.0-1613_git23fcb199_el7.x86_64.rpm -y
### S3:Post-Upgrade (All nodes)
    $/opt/seagate/cortx/s3/bin/s3_setup postupgrade
### Start the cluster (On primary node).
    $hctl bootstrap <CDF yaml file>