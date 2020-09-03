### License

Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For any questions about this software or licensing,
please email opensource@seagate.com or cortx-questions@seagate.com.

> -----------------------------------------------------------------------------------

# To setup dev vm

**Note current steps are assumed to be run within VM to be configured.**

Prerequisite

* Install and configure git as recommended by the provisioner.

In case of RHEL/CentOS 8 (Not needed for CentOS 7) Run
script to upgrade packages and enable repos
```sh
cd <s3 src>
./scripts/env/dev/upgrade-enablerepo.sh
```

Run setup script to configure dev vm
```sh
cd <s3 src>
./scripts/env/dev/init.sh
```

* At password prompts during init script run, please give below input
SSH password: <your SSH user password>
Enter new password for openldap rootDN:: seagate
Enter new password for openldap IAM admin:: ldapadmin

* Do NOT use following script directly on Real cluster as it is configured with fully qualified DNS.
For dev VM its safe to run following script to update host entries in /etc/hosts
```sh
./update-hosts.sh
```

Run Build and tests
```sh
./jenkins-build.sh
```

Above will run all system test over HTTPS to run over HTTP specify `--use_http`
```sh
./jenkins-build.sh --use_http
```

# To start s3server process
```sh
./dev-starts3.sh
```

* Make sure following processes/daemons are running before starting s3server
haproxy
s3authserver
slapd
cortx-motr

# How to start, stop or check status of s3server dependent processes/daemons:
```sh
systemctl start|stop|status haproxy
systemctl start|stop|status s3authserver
systemctl start|stop|status slapd
./third_party/motr/motr/st/utils/motr_services.sh start|stop
```

# To stop s3server
```sh
./dev-stops3.sh
```

# To build s3 rpms on dev setup
Install cortx-motr and cortx-motr-devel
```sh
yum install -y cortx-motr cortx-motr-devel
```

# To setup rpmbuild VM and Build S3 rpms
Run setup script to configure rpmbuild vm
```sh
cd <s3 src>
./scripts/env/rpmbuild/init.sh
```
Run following script to update host entries in /etc/hosts
```sh
cd <s3 src>
./update-hosts.sh
```

Install cortx-motr and cortx-motr-devel
```sh
yum install -y cortx-motr cortx-motr-devel
```
Obtain short git revision to be built.
```sh

git rev-parse --short HEAD
44a07d2
```

Build S3 rpm (here 44a07d2 is obtained from previous git rev-parse command)
./rpms/s3/buildrpm.sh -G 44a07d2

Build S3 rpm without motr rpm dependency
./rpms/s3/buildrpm.sh -a -G 44a07d2

Build s3iamcli rpm
./rpms/s3iamcli/buildrpm.sh -G 44a07d2

Built rpms will be available in ~/rpmbuild/RPMS/x86_64/

This rpms can be copied to release VM for testing.

# To Setup release VM
Run setup script to configure release vm

* Do NOT use this script directly on Real cluster as it cleans existing openldap
setup. You can use steps within init.sh cautiously.
For new/clean VM this is safe.

Ensure you have static hostname setup for the node. Example:
```sh
hostnamectl set-hostname s3release
# check status
hostnamectl status
```

* Do NOT use following script directly on real cluster as it is configured with fully
qualified DNS. Run following script to update host entries in /etc/hosts
```sh
cd <s3 src>
./update-hosts.sh
```

```sh
cd <s3 src>
./scripts/env/release/init.sh
```
VM is now ready to install cortx-motr, cortx-hare, cortx-s3server, cortx-s3iamcli and configure.

```sh
yum install -y cortx-hare cortx-motr cortx-s3server cortx-s3iamcli s3cmd
```

Once cortx-s3server rpm is installed, run following script to update ldap password
in authserver config. [ -l <ldap passwd> -p <authserver.properties file path> ]

/opt/seagate/cortx/auth/scripts/enc_ldap_passwd_in_cfg.sh -l ldapadmin \
    -p /opt/seagate/cortx/auth/resources/authserver.properties
