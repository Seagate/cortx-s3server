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

## Steps to generate multidomain ssl certificate in single file

# By default we are using the static certificate files which are located at <s3 src>/ansible/files/certs directory
1. stx-s3/s3 directory has s3server certificates which contains s3server.cert, s3server.pem, ca.cert etc
2. stx-s3-clients/s3 directory has client certificate i.e. ca.cert

- In developement environment, these files will be copied to /etc/ssl directory while running the ansible script.
- In production environment, we have stored these files at provisioner script which will be copied to /etc/ssl directory
- Haproxy.cfg file will use /etc/ssl/stx-s3/s3/s3server.pem file as ssl certificates for both s3server and authserver mentioned in frontend section.
- Any client e.g. s3iamcli, awscli should use ca certificate as /etc/ssl/stx-s3-clients/s3/ca.crt while using the ssl endpoints.
- s3server will not generate the certificate using s3certs rpm build.

## Generate fresh mulitdomain ssl certificates, please run below script
```sh
$ cd <s3 src>/scripts/ssl
$ ./generate_multidomain_certificate.sh -f multidomain_input.conf
```
- This will generate the certificates in <s3 src>scripts/ssl/s3_certs_sandbox folder

## Encrypt and decrypt ldap password by authserver
-we are using static s3authserver.jks file to encrypt and decrypt the ldap password.
-keystore file location is /opt/seagate/cortx/auth/resources/s3authserver.jks

