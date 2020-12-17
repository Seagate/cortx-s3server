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

## Steps to Configure & run Splunk compliance test.

Pre-requiste:

Splunk s3test should be installed and configured on the system.

Steps:

1. Virtualenv should be installed on the system
   yum install python-virtualenv
   
## Prepare Environment to run tests:


1. Clone the repo of splunk.
   
   git clone https://github.com/splunk/s3-tests)
   
   Once cloned,git checkout below commit id:
   3dc9362b1d322a59bd4e8f207d5a94070502b78b 
   
2. Run bootstrap to download and set up python virtual environment to run the tests.
   
   ~/path_to_repo/splunk/bootstrap

3. After above script runs it can be seen that a folder is created with name virtualenv . Switch to this virtual environment.
   
   source ~/path_to_repo/splunk/s3-tests/virtualenv/bin/activate

4. Configure splunk.conf (s3-tests/splunk.conf ).
   You will need to configure splunk.conf with the location of the service and two different credentials. 
   Usually you just need to configure it ONCE(unless the host changes, access key pairs rotates, etc)
   
   splunk.conf.SAMPLE is present under ~/path_to_repo/splunk/s3-tests
   
5. To disable SSL certificate verification export following variables.

   export PYTHONHTTPSVERIFY=0

6. Splunk s3 tests uses AWS signature version 2 by default .If software under test uses signature version 4 then an environment variable need to be defined explicitly.
   
   export S3_USE_SIGV4=1


## Automation Splunks3test script : splunk_script.sh

 It runs test using v2 and v4 signature.
 Test files used for same are :cephtestfilev2.txt and cephtestfilev4.txt.
 Runlogs are captured under /root/logs/splunk-logs directory.
 Two logs are generated ,one for v2 and v4 run.
