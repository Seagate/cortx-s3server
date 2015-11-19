# For now dev centric readme

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

## How to run s3 server (this current assumes all dependencies are on same local VM)
```sh
sudo /opt/seagate/s3/starts3.sh
```

## How to run auth server (this current assumes all dependencies are on same local VM)
```sh
sudo /opt/seagate/s3/startauth.sh
```
