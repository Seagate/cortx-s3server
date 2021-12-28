# Pre-Requisites:
1. Install third-party packages
    >
    >`curl -s http://cortx-storage.colo.seagate.com/releases/cortx/third-party-deps/rpm/install-cortx-prereq.sh | bash`
    >
2. Add below repositories on the test VM (Generate your own [jenkins custom build](http://eos-jenkins.colo.seagate.com/job/GitHub-custom-ci-builds/job/centos-7.8/job/cortx-custom-ci/)) using 'main' branch of all components 
    1. lustre (for cortx-motr)
      >`$yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/github/integration-custom-ci/centos-7.8.2003/custom-build-399/3rd_party/lustre/custom/tcp/`
    2. cortx iso
      >`$yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/github/integration-custom-ci/centos-7.8.2003/custom-build-399/cortx_iso/` 
    3. 3rd part libraries
      >`$yum-config-manager --add-repo=http://cortx-storage.colo.seagate.com/releases/cortx/github/integration-custom-ci/centos-7.8.2003/custom-build-1120/3rd_party/`
3. S3 Server rpm
    >`$yum install -y --nogpgcheck cortx-s3server`
4. Machine-ID of the VM
    >`$cat /etc/machine-id`
5. FQDN of the VM
    >`$hostname`
6. Cluster-ID
    >`$cat /opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml`
   Please edit this file (and template file in step 8) if the non-default cluster-ID is used
7. Openldap credentials
    1. `$s3cipher generate_key --const_key cortx`
    2. `$s3cipher encrypt --data "any-string-of-your-choice as LDAP root secret key" --key 'output of 7(i) step'`
    3. `$s3cipher encrypt --data "any-string-of-your-choice as SGIAM secret key" --key 'output of 7(i) step'`

8. Update following s3server confstore template files (refer respective `*.sample` files for help)
    1. `/opt/seagate/cortx/s3/conf/s3.config.tmpl.1-node`
        1. Replace TMPL_MACHINE_ID with machine-id of the VM (**step 4**)
        2. Replace TMPL_CLUSTER_ID with cluster_id from **step 6**
        3. Replace TMPL_HOSTNAME with FQDN of the VM
        4. Replace TMPL_ROOT_SECRET_KEY with output of **step 7(ii)**
        5. Replace TMPL_SGIAM_SECRET_KEY with output of **step 7(iii)**
    2. `/opt/seagate/cortx/s3/conf/s3.init.tmpl.1-node`
        1. Replace TMPL_MACHINE_ID with machine-id of the VM (**step 4**)
        2. Replace TMPL_CLUSTER_ID with cluster_id from **step 6**
        3. Replace TMPL_HOSTNAME with FQDN of the VM
        4. Replace TMPL_ROOT_SECRET_KEY with output of **step 7(ii)**
        5. Replace TMPL_SGIAM_SECRET_KEY with output of **step 7(iii)**
    3. `/opt/seagate/cortx/s3/conf/s3.test.tmpl.1-node`
        1. Replace TMPL_SGIAM_SECRET_KEY with output of **step 7(iii)**


    * Add/set **'srvnode-1.data.public'** to entry containing 'Public IP' in `/etc/hosts` file
    * Add/set **'srvnode-1.data.private'** to entry containing 'private IP' in `/etc/hosts` file

* If found, comment the lines containing "PROFILE=SYSTEM" from /etc/haproxy/haproxy.cfg file
* If not existing, create /etc/ssl/stx/stx.pem file and copy contents from here : [stx.pem](https://raw.githubusercontent.com/Seagate/cortx-prvsnr/main/srv/components/misc_pkgs/ssl_certs/files/stx.pem) 
* (Optional) If you need client certificates on your client node, create /etc/ssl/stx-s3-clients/s3/ca.crt file and copy contents from here : [ca.crt](https://raw.githubusercontent.com/Seagate/cortx-prvsnr/main/srv/components/s3clients/files/ca.crt) on your client machine.
* Follow all the **pre-requisite** steps for [cortx-py-utils](https://github.com/Seagate/cortx-utils/wiki/%22cortx-py-utils%22-single-node-manual-provisioning#pre-requisites). Mini-provisioning for s3server and utils will happen alongside as per below steps. 
Reference wiki for cortx-py-utils - [cortx-py-utils-single-node-manual-provisioning](https://github.com/Seagate/cortx-utils/wiki/%22cortx-py-utils%22-single-node-manual-provisioning#sample-json-file-representing-confstore-keys)

# S3server Mini Provisioning
### S3:Post_Install
    $/opt/seagate/cortx/utils/bin/utils_setup post_install --config yaml:///tmp/utils.post_install.tmpl.1-node
    $/opt/seagate/cortx/s3/bin/s3_setup post_install --config "yaml:///opt/seagate/cortx/s3/conf/s3.post_install.tmpl.1-node"
### S3:Prepare
    $/opt/seagate/cortx/utils/bin/utils_setup prepare --config yaml:///tmp/utils.prepare.tmpl.1-node
    $/opt/seagate/cortx/s3/bin/s3_setup prepare --config "yaml:///opt/seagate/cortx/s3/conf/s3.prepare.tmpl.1-node"
### S3:Config
* All 3rd party services are expected to be run before config stage as per [CORTX Components Mini Provisioning Deliverables](https://seagate-systems.atlassian.net/wiki/spaces/PRIVATECOR/pages/84803862/CORTX+Components+Mini+Provisioner+Deliverables) 
* Use [s3prov_start_services.sh](https://github.com/Seagate/cortx-s3server/blob/main/scripts/s3prov_start_services.sh) to run required 3rd party services
    >`$sh ./s3prov_start_services.sh haproxy slapd rsyslog sshd`
* >`/opt/seagate/cortx/utils/bin/utils_setup config --config yaml:///tmp/utils.config.tmpl.1-node`
* >`$/opt/seagate/cortx/s3/bin/s3_setup config --config "yaml:///opt/seagate/cortx/s3/conf/s3.config.tmpl.1-node"`
### S3:Init
    $/opt/seagate/cortx/utils/bin/utils_setup init --config yaml:///tmp/utils.init.tmpl.1-node
    $/opt/seagate/cortx/s3/bin/s3_setup init --config "yaml:///opt/seagate/cortx/s3/conf/s3.init.tmpl.1-node"

# Start s3server and motr for I/O 
1. Install Hare
    >
    >`$yum-config-manager --add-repo https://rpm.releases.hashicorp.com/RHEL/hashicorp.repo`
    >
    >`$yum -y install consul-1.7.8`
    >
    >`$yum install -y --nogpgcheck cortx-hare`
    > 
2. Set hostname as node name
    >
    >`$hostname > /var/lib/hare/node-name`
    >
3. Create virtual devices
    >
    >`$m0setup`
4. Configure lnet
    * Create file: /etc/modprobe.d/lnet.conf
    * Add below line in the file:
        > options lnet networks=tcp(eth0) config_on_load=1
    * Start lnet service
        >`$service lnet start`
5. Create CDF file: /tmp/singlenode.yaml as below
    https://seagatetechnology.sharepoint.com/:u:/r/sites/gteamdrv1/tdrive1224/Shared%20Documents/Components/S3/mini-provisioning/singlenode.yaml?csf=1&web=1&e=bwNpy8
    * Note: please mention confstore config file's 's3_instances' value for 'm0_clients:s3' in above CDF file
6. bootstrap hctl
    > 
    >`$hctl bootstrap --mkfs /tmp/singlenode.yaml`
    >
7. Check status
    >
    >`$hctl status`
    >
8. Start s3authserver
    >
    >`$systemctl restart s3authserver.service`
    >
9. Start s3background services
    >
    >`$systemctl start s3backgroundproducer`
    >
    >`$systemctl start s3backgroundconsumer`
    >
10. Add below entries in /etc/hosts file of the client node
    > iam.seagate.com
    > s3.seagate.com
    * Sample entry in /etc/hosts:
    > \<public IP of Server node\>  iam.seagate.com s3.seagate.com
    >
11. To install s3iamcli:
    >
    > $yum-config-manager --add-repo=http://cortx-storage.colo.seagate.com/releases/cortx/uploads/centos/centos-7.8.2003/s3server_uploads/
    >
    >$yum install --nogpgcheck cortx-s3iamcli
    >
    > iamadmin password would be 'ldapadmin'
    >
12. If you want to use SSL, please copy [ca.crt](https://github.com/Seagate/cortx-prvsnr/blob/pre-cortx-1.0/srv/components/s3clients/files/ca.crt) file to /etc/ssl/stx-s3-clients/s3/ on the VM.
    >
    > Add the path to ca.crt file in s3-clients config files, like aws (/root/.aws/config), s3iamcli(/root/.sgs3iamcli/config.yaml) etc.
    >

# Optional Test and Teardown (Mini-Provisioning) steps
### S3:Test
    1. Add third party repo using this command yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/uploads/centos/centos-7.8.2003/
    2. Install test RPM if not already installed.
    3. Make sure /etc/hosts is properly configured.
    4. $/opt/seagate/cortx/s3/bin/s3_setup test --config "yaml:///opt/seagate/cortx/s3/conf/s3.test.tmpl.1-node"
### S3:Reset
    $/opt/seagate/cortx/s3/bin/s3_setup reset --config "yaml:///opt/seagate/cortx/s3/conf/s3.reset.tmpl.1-node"
    $/opt/seagate/cortx/utils/bin/utils_setup reset --config yaml:///tmp/utils.reset.tmpl.1-node

### S3:Cleanup
    $/opt/seagate/cortx/s3/bin/s3_setup cleanup --config "yaml:///opt/seagate/cortx/s3/conf/s3.cleanup.tmpl.1-node"
    $/opt/seagate/cortx/utils/bin/utils_setup cleanup --config yaml:///tmp/utils.cleanup.tmpl.1-node    


# Optional Pre-Upgrade and Post-Upgrade steps
### Stop the cluster
    $hctl shutdown
### S3:Pre-Upgrade
    $/opt/seagate/cortx/s3/bin/s3_setup preupgrade
### S3:RPM-Upgrade
    $yum upgrade cortx-s3server-2.0.0-1613_git23fcb199_el7.x86_64.rpm -y
### S3:Post-Upgrade
    $/opt/seagate/cortx/s3/bin/s3_setup postupgrade
### Start the cluster
    $hctl bootstrap /tmp/singlenode.yaml
    


# Troubleshooting
1. Note s3_setup is not Idempotent as of now. In case of configuration failure or re-configuration, Perform following steps:
    1. Run S3:Reset step
    2. Run S3:Cleanup step
    3. copy [clean_openldap script](https://github.com/Seagate/cortx-s3server/blob/main/scripts/provisioning/clean_openldap.sh) to the VM, and execute it.
    4. Repeat all steps of the s3server mini-provisioning, starting from `post_install`, till `init`
