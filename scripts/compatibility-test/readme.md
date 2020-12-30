
## Steps to configure & run compatibility test.

Pre-requisites:
   1. S3Server should be up and running 
   
Steps:

1. Navigate to the compatibility-test script folder
   ```
      cd cortx-s3server/scripts/compatibility-test
   ```

2. create s3test.conf for the corresponding integration. Refer sample template for ceph and splunk integrations in the respective folders. s3 account credentials need to be updated in the configurations (2 account for splunk / 3 accounts for ceph)


3. Execute run-test.sh with arguments 

      - Run command format
      ```
         sh run-test.sh -c=conf_location -i=integration_type (i.e splunk | ceph) -t=test_type
      ```
      
      - Run ceph tests
      ```
         sh run-test.sh -c=ceph/ceph.conf -i=ceph
      ```
      - Run Splunk tests
      
      ```
         sh run-test.sh -c=splunk/splunk.conf -i=splunk
      ```
      
      - Run V2 or V4 test only 
      ```
         sh run-test.sh -c=ceph/ceph.conf -i=ceph -t=v4
         sh run-test.sh -c=ceph/ceph.conf -i=ceph -t=v2
      ```

      Run command help
      ```
         sh run-test --help
      ```

Reference:

1. ceph test - https://github.com/ceph/s3-tests/
2. splunk test -  https://github.com/splunk/s3-tests/
