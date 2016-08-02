# For now dev centric readme

# Updation to nginx.conf.sample -- Added client_max_body_size to have max body size as 5GB

## How to install clang-format
Install clang-format for code formatting.
```sh
cd ~/Downloads/
wget http://llvm.org/releases/3.8.0/clang+llvm-3.8.0-linux-x86_64-centos6.tar.xz
tar -xvJf clang+llvm-3.8.0-linux-x86_64-centos6.tar.xz
sudo ln -s ~/Downloads/clang+llvm-3.8.0-linux-x86_64-centos6/bin/clang-format /bin/clang-format
```

## How to install git-clang-format
Install git-clang-format to run clang-format only on new changes.
```sh
cd ~/Downloads/
wget https://raw.githubusercontent.com/llvm-mirror/clang/master/tools/clang-format/git-clang-format
chmod +x git-clang-format
sudo cp git-clang-format /usr/bin
git config --global clangFormat.style 'Google'
```

## How to use git clang-format
Once you are ready with your changes to be committed, use below sequence of commands:
```sh
git clang-format --diff <files>   //prints the changes clang-format would make in next command
git clang-format -f <files>       //makes formatting changes in specified files
git add <files>
git commit
```

## How to inform clang-format to ignore code for formatting
If you don't want clang-format to work on a section of code then surround it with `// clang-format off` and `// clang-format on`
```cpp

#include <iostream>
int main() {
// clang-format off
  std::cout << "Hello world!";
  return 0;
// clang-format on
}

```

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

## How to run S3 server Unit tests?
```sh
bazel test //:s3ut  --define MERO_SRC=`pwd`/../..
```
or
```sh
bazel-bin/s3ut
```

## How to run S3 server System tests?
```sh
cd st/clitests/
cat readme
```

## How to run Auth server tests?
```sh
mvn test
```

## How to run S3 server standard (ST + UT) tests?
Read above steps to setup and build ST + UT
```sh
./runalltest.sh
```

## How to install S3 server and Auth server
Install build to /opt
```sh
sudo ./makeinstall
```

```sh
sudo cp auth/s3authserver.service /lib/systemd/system
sudo systemctl daemon-reload
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
sudo systemctl start authserver
```

## How to stop auth server (this current assumes all dependencies are on same local VM)
```sh
sudo systemctl stop authserver
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
