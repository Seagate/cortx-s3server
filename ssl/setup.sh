#!/bin/sh

rm -rf ssl_sandbox
mkdir -p ssl_sandbox
cd ssl_sandbox

printf "Creating root CA certificate for generating self signed certificates...\n"
openssl req -new -x509 -extensions v3_ca  -keyout ca.key -out ca.crt -days 3650
printf "\nca.key - private key for root CA\n"
printf "ca.crt - public key certificate for root CA to be used by clients\n"

printf "\nCreating private key for domain certificates...\n"
openssl genrsa -out s3.seagate.com.key 1024

printf "\nCreating certificate signing request...\n"
openssl req -new -key  s3.seagate.com.key -out s3.seagate.com.csr -config ../openssl.cnf

printf "\nCreating the ssl certificate for domain s3.seagate.com...\n"
openssl x509 -req -days 365 -in s3.seagate.com.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out s3.seagate.com.crt -extfile ../openssl.cnf -extensions v3_ca

cat s3.seagate.com.crt ca.crt > ca.pem

printf -- "\n\n*********************************************************************\n"
printf -- "*********************************************************************\n"
printf -- "---------------------------------------------------------------------\n"
printf "Copy the following certificates to setup ssl for nginx:\n"
printf "cp ssl_sandbox/s3.seagate.com.* /etc/nginx/ssl/seagate/s3/\n"
printf "cp ssl_sandbox/ca.*             /etc/nginx/ssl/seagate/s3/\n"
printf -- "---------------------------------------------------------------------\n"
read -p "Press [Enter] key to continue..."

printf -- "---------------------------------------------------------------------\n"
printf "\nUpdate following entries in /etc/nginx/nginx.conf and restart nginx\n"
printf "server {\n"
printf "    ssl                   on;\n"
printf "    listen                443 reuseport;\n"
printf "    server_name           s3.seagate.com *.s3.seagate.com;\n"
printf "    ssl_certificate       /etc/nginx/ssl/seagate/s3/ca.pem;\n"
printf "    ssl_certificate_key   /etc/nginx/ssl/seagate/s3/s3.seagate.com.key;\n"
printf "   ...\n"
printf -- "---------------------------------------------------------------------\n"
read -p "Press [Enter] key to continue..."

printf -- "---------------------------------------------------------------------\n"
printf "To use ssl with s3cmd:\n"
printf "Update following attributes in the .s3cfg files, ex: pathstyle.s3cfg\n"
printf "ca_certs_file = /etc/nginx/ssl/seagate/s3/ca.pem\n"
printf "check_ssl_certificate = True\n"
printf "use_https = True\n"
printf -- "---------------------------------------------------------------------\n"
read -p "Press [Enter] key to continue..."

printf -- "---------------------------------------------------------------------\n"
printf "To use ssl with jclient/jcloud tests:\n"
printf "Run $ /opt/jdk1.8.0_91/bin/keytool -import -trustcacerts -keystore /opt/jdk1.8.0_91/jre/lib/security/cacerts -storepass changeit -noprompt -alias s3server -file /etc/nginx/ssl/seagate/s3/s3.seagate.com.crt\n"
read -p "Press [Enter] key to continue..."

printf "\nUpdate the endpoints in auth-utils/jcloudclient/src/main/resources/endpoints.properties\n"
printf "and auth-utils/jclient/src/main/resources/endpoints.properties to use https url\n"
printf "\nRebuild jclient and jcloud clients and copy to st/clitests.\n"
printf -- "---------------------------------------------------------------------\n"
read -p "Press [Enter] key to continue..."

printf "Verify the certificate with $ openssl x509 -text -in s3.seagate.com.crt\n\n"

printf "Verify the nginx ssl setup using command:\n"
printf "$ openssl s_client -connect s3.seagate.com:443 -showcerts | openssl x509 -noout -text\n\n"
printf "Output should have DNS:s3.seagate.com, DNS:*.s3.seagate.com under X509v3 Subject Alternative Name:\n\n"
read -p "Press [Enter] key to continue..."

printf "Currently only tests for bucket name with '.' are not supported due to s3cmd limitations.\n"
printf "Comment out tests using seagate.bucket in s3cmd_specs.py\n"

cd ..

