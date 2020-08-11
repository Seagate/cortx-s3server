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


## Tasks supported
* Dry run to recover possible S3 Bucket Metadata corruption
* Recover detected S3 Bucket Metadata corruption

**Python Version 3.6 required.**

* Installing Python 3.6 and required dependencies
    1. yum install wget epel-release
    2. yum install python36

**S3backgroundelete must be configured before using s3recovery tool**
* s3recoverytool uses cortx-s3 APIs defined in s3backgrounddelete, so these
  must be installed before using s3recovery tool.

**Installing S3backgroundelete cortx-S3 APIs**
  1. ```sh
     cd 's3server-repo-root'/s3backgrounddelete
     ```
  2. ```sh
     python36 setup.py clean
     ```
  3. ```sh
     python36 setup.py build
     ````
  4. ```sh
     python36 setup.py install
     ```

## Configuring s3recovery tool

  1. ```sh
     cd 's3server-repo-root'/s3recovery
     ```
  2. For cleaning:
     ```sh
     python36 setup.py clean
     ```
  3. For building and installing:
     ```sh
     python36 setup.py build
     python36 setup.py install
     ```
  4. Create S3 account with name *s3-recovery-svc* and update *access_key* and
     *secret_key* in _/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml_

# S3-Metadata recovery tool

## usage: s3recovery [-h] [--dry_run] [--recover]

* optional arguments:
  * -h, --help  show this help message and exit
  * --dry_run   Dry run of S3-Metadata corruption recovery
  * --recover   Recover S3-Metadata corruption (Silent)

* logs are generated at _/var/log/seagate/s3/s3recovery/_

