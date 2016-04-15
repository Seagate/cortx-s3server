# For now dev centric readme

# Updation to nginx.conf.sample -- Added client_max_body_size to have max body size as 5GB

## How to Build S3 server?

Fetch the third party dependencies.
```sh
./fetch_thirdparty.sh
```

## How to build S3 server?
Run Bazel build. For details see bazel BUILD file.
```sh
bazel build //:s3server  --define MERO_SRC=`pwd`/../..
```

## How to build Auth server?
```sh
cd auth
mvn package
```

## How to build S3 server tests?
```sh
cd third_party
./setup_libevhtp.sh test
cd ..
bazel build //:s3ut  --define MERO_SRC=`pwd`/../..
```

## How to run S3 server tests?
```sh
bazel test //:s3ut  --define MERO_SRC=`pwd`/../..
```
or
```sh
bazel-bin/s3ut
```

## How to run Auth server tests?
```sh
mvn test
```

## How to install S3 server and Auth server
Install build to /opt
```sh
sudo ./makeinstall
```

## How to run s3 server daemon(this current assumes all dependencies are on same local VM)
```sh
sudo /opt/seagate/s3/starts3.sh
```

## How to stop s3 server daemon
```sh
sudo /opt/seagate/s3/stops3.sh
```

## How to run auth server (this current assumes all dependencies are on same local VM)
```sh
sudo /opt/seagate/s3/startauth.sh
```


## Steps to create Java key store and Certificate.
```sh
keytool -genkey -keyalg RSA -alias s3auth -keystore s3_auth.jks -storepass seagate -validity 360 -keysize 2048
```
What is your first and last name?
   [Unknown]: signin.seagate.com
What is the name of your organization unit?
   [Unknown]: s3
What is the name of your organization?
   [Unknown]: seagate
What is the name of your City or Locality?
   [Unknown]: Pune
What is the name of your State or Province?
   [Unknown]: MH
What is the two-letter country code for this unit?
   [Unknown]: IN
is CN=signin.seagate.com, OU=s3, O=seagate, L=Pune, ST=MH, C=IN correct?
   [no]: yes

Enter key password for <s3auth>
	(RETURN if same as keystore password):

## Steps to generate crt from Key store
```sh
keytool -importkeystore -srckeystore s3_auth.jks -destkeystore s3_auth.p12 -srcstoretype jks -deststoretype pkcs12
```

```sh
openssl pkcs12 -in s3_auth.jks.p12 -out s3_auth.jks.pem
```

```sh
openssl x509 -in seagates3.pem -out seagates3.crt
```sh

## RPM builds - For S3 deployment you need 3 rpms, libuv, libcassandra, and s3 server
Following dependencies are required to build rpms.
yum install ruby
yum install ruby-devel
gem install bundler
gem install fpm
```

Generate RPMs
```sh
cd third_party
./setup_libuv.sh
# Above should generate a rpm file inside libuv
./setup_libcassandra.sh
# Above should generate a rpm file inside cpp-driver
cd ..

# Ensure you have run makeinstall to create necessary S3 layout for deployment in /opt/seagate
./makerpm
# Above should generate a rpm file in current folder

```
