
## For now dev centric readme

## How to Build?

Fetch the third party dependencies.
./fetch_thirdparty.sh

## How to build?
Run Bazel build. For details see bazel BUILD file.
```sh
bazel build //:s3server  --define MERO_SRC=`pwd`/../..
```

## How to build tests?
```sh
bazel build //:s3ut  --define MERO_SRC=`pwd`/../..
```

## How to run tests?
```sh
bazel test //:s3ut  --define MERO_SRC=`pwd`/../..
```
or
```sh
bazel-bin/s3ut
```

## How to install S3 server
Install build to /opt
```sh
sudo ./makeinstall
```

## How to run s3 server (this current assumes all dependencies are on same local VM)
```sh
sudo /opt/seagate/s3/starts3.sh
```
