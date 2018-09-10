#!/bin/bash
set -e

read -p "Enter S3 endpoint (default is s3.seagate.com): " s3_default_endpoint
read -p "Enter S3 region endpoint (default is s3-us-west-2.seagate.com): " s3_region_endpoint
read -p "Enter S3 iam endpoint (default is iam.seagate.com): " s3_iam_endpoint
read -p "Enter S3 sts endpoint (default is sts.seagate.com): " s3_sts_endpoint
read -p "Enter Open ldap domain name (default is localhost): " openldap_domainname
read -p "Enter the key store passphrase (default is seagate): " passphrase

[ "$s3_default_endpoint" == "" ] && s3_default_endpoint="s3.seagate.com"
[ "$s3_region_endpoint" == "" ] && s3_region_endpoint="s3-us-west-2.seagate.com"
[ "$s3_iam_endpoint" == "" ] && s3_iam_endpoint="iam.seagate.com"
[ "$s3_sts_endpoint" == "" ] && s3_sts_endpoint="sts.seagate.com"
[ "$openldap_domainname" == "" ] && openldap_domainname="localhost"
[ "$passphrase" == "" ] && passphrase="seagate"

function generate_s3_certs()
{
  # create dns.list file in ssl folder with given domain names.
  # generate ssl certificate and key
  ssl_sandbox="$CURRENT_DIR/ssl_sandbox"
  if [ -d "$ssl_sandbox" ]
  then
    echo "removing existing directory: $ssl_sandbox"
    rm -rf $ssl_sandbox
  fi

  # create dns list files
  dns_list_file="$CURRENT_DIR/dns.list"
  rm -f $dns_list_file
  echo $s3_default_endpoint > $dns_list_file
  echo "*.$s3_default_endpoint" >> $dns_list_file
  echo $s3_region_endpoint | tr , '\n' >> $dns_list_file

  # generate s3 ssl cert files
  $CURRENT_DIR/setup-ssl.sh --san-file $dns_list_file
  cat ${ssl_sandbox}/${s3_default_endpoint}.crt ${ssl_sandbox}/${s3_default_endpoint}.key > ${ssl_sandbox}/${s3_default_endpoint}.pem
  \cp $ssl_sandbox/* $s3_dir

  # cleanup
  rm -f $dns_list_file
  rm -rf $ssl_sandbox
}

function generate_jks_and_iamcert()
{
  #san_region_endpoint=`echo $s3_region_endpoint | sed 's/,/,dns:/g'`
  san_list="dns:$s3_iam_endpoint,dns:$s3_sts_endpoint"

  keytool -genkeypair -keyalg RSA -alias s3auth -keystore ${s3auth_dir}/s3_auth.jks \
  -storepass ${passphrase} -keypass ${passphrase} -validity 3600 -keysize 2048 \
  -dname "C=IN, ST=Maharashtra, L=Pune, O=Seagate, OU=S3, CN=$s3_iam_endpoint" -ext SAN=$san_list

  ## Steps to generate crt from Key store
  keytool -importkeystore -srckeystore ${s3auth_dir}/s3_auth.jks \
  -destkeystore ${s3auth_dir}/s3_auth.p12 -srcstoretype jks \
  -deststoretype pkcs12 -srcstorepass ${passphrase} -deststorepass ${passphrase}

  openssl pkcs12 -in ${s3auth_dir}/s3_auth.p12 -out ${s3auth_dir}/s3_auth.jks.pem \
  -passin pass:${passphrase} -passout pass:${passphrase}
  openssl x509 -in ${s3auth_dir}/s3_auth.jks.pem -out ${s3auth_dir}/${s3_iam_endpoint}.crt

  #Extract Private Key
  openssl pkcs12 -in ${s3auth_dir}/s3_auth.p12 -nocerts -out ${s3auth_dir}/s3_auth.jks.key \
  -passin pass:${passphrase} -passout pass:${passphrase}
  #Decrypt using pass phrase
  openssl rsa -in ${s3auth_dir}/s3_auth.jks.key -out ${s3auth_dir}/${s3_iam_endpoint}.key \
  -passin pass:${passphrase}
  cat ${s3auth_dir}/${s3_iam_endpoint}.crt ${s3auth_dir}/${s3_iam_endpoint}.key > \
  ${s3auth_dir}/${s3_iam_endpoint}.pem

  ## Steps to create Key Pair for password encryption and store it in java keystore
  ## This key pair will be used by AuthPassEncryptCLI and AuthServer for encryption and
  ## decryption of ldap password respectively
  keytool -genkeypair -keyalg RSA -alias s3auth_pass -keystore ${s3auth_dir}/s3_auth.jks \
  -storepass ${passphrase} -keypass ${passphrase} -validity 3600 -keysize 512 \
  -dname "C=IN, ST=Maharashtra, L=Pune, O=Seagate, OU=S3, CN=$iam_end_point" -ext SAN=$san_list

  # import open ldap cert into jks
  keytool -import -trustcacerts -keystore ${s3auth_dir}/s3_auth.jks \
  -storepass ${passphrase} -noprompt -alias ldapcert -file ${openldap_dir}/localhost.crt
  # import haproxy s3 ssl cert into jks file
  keytool -import -trustcacerts -keystore ${s3auth_dir}/s3_auth.jks \
  -storepass ${passphrase} -noprompt -alias s3 -file ${s3_dir}/${s3_default_endpoint}.crt
}

function generate_openldap_cert()
{
  # create dns.list file in ssl folder with localhost.
  # generate ssl certificate and key
  ssl_sandbox="$CURRENT_DIR/ssl_sandbox"
  if [ -d "$ssl_sandbox" ]
  then
    echo "removing existing directory: $ssl_sandbox"
    rm -rf $ssl_sandbox
  fi

  # create dns list file
  dns_list_file="$CURRENT_DIR/dns.list"
  rm -f $dns_list_file
  echo "$openldap_domainname" > $dns_list_file

  # generate openldap cert files
  $CURRENT_DIR/setup-ssl.sh --san-file $dns_list_file
  \cp $ssl_sandbox/* $openldap_dir
  rm -f $dns_list_file
  rm -rf $ssl_sandbox

  # ./enable_ssl_openldap.sh -cafile $openldap/ca.crt \
  # -certfile $openldap/localhost.crt -keyfile $openldap/localhost.key
  # Update ldap.conf so that ldap client cli can connect to ssl port of ldap i.e ldaps
  echo "#
## LDAP Defaults
##
#
## See ldap.conf(5) for details
## This file should be world readable but not world writable.
#
##BASE   dc=example,dc=com
SSL on
##SIZELIMIT      12
##TIMELIMIT      15
##DEREF          never
TLS_CACERTDIR     /etc/ssl/openldap
TLS_CACERT        /etc/ssl/openldap/ca.crt
TLS_REQCERT allow
#
## Turning this off breaks GSSAPI used with krb5 when rdns = false
SASL_NOCANON    on" > $openldap_dir/ldap.conf
}

CURRENT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
sand_box="$CURRENT_DIR/sandbox"

# create target directories
rm -rf $sand_box
mkdir -p $sand_box
s3_dir="$sand_box/s3"
mkdir -p $s3_dir
s3auth_dir="$sand_box/s3auth"
mkdir -p $s3auth_dir
openldap_dir="$sand_box/openldap"
mkdir -p $openldap_dir

# 1. genearte ssl certificates
generate_s3_certs
# 2. generate openldap cert
generate_openldap_cert
# 3. generate jks
generate_jks_and_iamcert

echo
echo "##Certificate generation is done##"
