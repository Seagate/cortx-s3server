* Note current steps are assumed to be run from a development VM.

# install pip
```sh
easy_install pip
```

* If easy_install is not installed
```sh
yum install python-setuptools python-setuptools-devel
```

* AWS CLI is installed as part of jenkins build job

# Install awscli-plugin-endpoint
```sh
pip install awscli-plugin-endpoint
```

# Create S3 Account using s3iamcli tool

* s3iamcli gets installed as part of jenkins-build.sh run.
Make sure that s3iamcli directory is in your PATH environment variable.
It usually gets installed in /usr/local/bin directory.
Also s3authserver should be running.

```sh
s3iamcli CreateAccount -n <account-name> -e <email-address> --ldapuser sgiamadmin --ldappasswd ldapadmin
```

* Make sure AWS S3 CLI is installed
```sh
aws --version
```

# configure s3 cli
```sh
aws configure
```

* Use AccessKeyId and SecretKey obtained from 's3iamcli CreateAccount' command run above in aws configuration.
Give region as US.

# Set Endpoints
```sh
aws configure set plugins.endpoint awscli_plugin_endpoint
aws configure set s3.endpoint_url https://s3.seagate.com
aws configure set s3api.endpoint_url https://s3.seagate.com
```

* Default aws configuration file location: ~/.aws/config

* For SSL connection, add CA Cert file location in AWS config file
ca_bundle = <path/to/ca-cert/file>







