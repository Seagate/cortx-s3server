# Setup VM
Import VM from (use latest or check with team)
http://jenkins.mero.colo.seagate.com/share/bigstorage/sage_releases/vmdk_images/

- sage-CentOS-7.5.x86_64-7.5.0_3-k3.10.0.vmdk
-- source:
http://jenkins.mero.colo.seagate.com/share/bigstorage/sage_releases/vmdk_images/sage-CentOS-7.5.x86_64-7.5.0_3-k3.10.0.vmdk

Import VM into VMWare Fusion or VirtualBox

Clone source on the new VM.
  git clone http://gerrit.mero.colo.seagate.com:8080/s3server
or follow
https://docs.google.com/document/d/17ufHPsT62dFL-5VE8r6NeADSefUa0eGgkmPNTkv-k3o/

Ensure you have at least 8GB RAM for dev VM and 4GB RAM for release/rpmbuild VM.

# To setup dev vm
Run setup script to configure dev vm
```sh
cd <s3 src>
./scripts/env/dev/init.sh
```

Run Build and tests
```sh
./jenkins-build.sh
```

# To setup rpmbuild VM and Build S3 rpms
Run setup script to configure rpmbuild vm
```sh
cd <s3 src>
./scripts/env/rpmbuild/init.sh
```

Install mero/halon
```sh
yum install -y halon mero mero-devel
```
* This installs latest from http://jenkins.mero.colo.seagate.com/share/bigstorage/releases/hermi/last_successful/)

Obtain short git revision to be built.
```sh
git rev-parse --short HEAD
44a07d2
```

Build S3 rpm (here 44a07d2 is obtained from previous git rev-parse command)
./rpms/s3/buildrpm.sh -G 44a07d2

Build s3iamcli rpm
./rpms/s3iamcli/buildrpm.sh -G 44a07d2

Built rpms will be available in ~/rpmbuild/RPMS/x86_64/

This rpms can be copied to release VM for testing.

# To Setup release VM
Install certificate rpms. These can either be build with following or obtained
from dev VM build.
Location on dev build post build: ~/rpmbuild/RPMS/x86_64/stx-s3-c*


Run setup script to configure rpmbuild vm
```sh
cd <s3 src>
./scripts/env/release/init.sh
```
