#!/bin/sh
# Script to generate S3 sepcs tar
set -e

TARGET_DIR="s3-specs"
SRC_ROOT="$(dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd ) " )"
AUTH_CLI_DIR="$TARGET_DIR/auth-utils/pyclient"
TEST_DIR="$TARGET_DIR/st/clitests"

mkdir -p "$AUTH_CLI_DIR/pyclient"
mkdir -p "$AUTH_CLI_DIR/config"
mkdir -p $TEST_DIR

# copy pyclient files
auth_cli_files="pyclient/access_key.py pyclient/account.py \
    pyclient/assume_role_with_saml.py pyclient/config.py \
    pyclient/federation_token.py pyclient/group.py pyclient/__main__.py \
    pyclient/policy.py pyclient/role.py pyclient/saml_provider.py \
    pyclient/user.py pyclient/util.py config/controller_action.yaml \
    config/endpoints.yaml"
for file in $auth_cli_files
do
    cp -r "$SRC_ROOT/auth-utils/pyclient/$file" "$AUTH_CLI_DIR/$file"
done

# copy jclient.jar
cp "$SRC_ROOT/auth-utils/jclient/target/jclient.jar" "$TEST_DIR"

# copy jcloudclient.jar
cp "$SRC_ROOT/auth-utils/jcloudclient/target/jcloudclient.jar" "$TEST_DIR"

# copy test framework and spec files
test_files="ldap.py ldap_setup.py auth.py auth_spec_param_validation.py \
    auth_spec_negative_and_fi.py auth_spec_signature_calculation.py \
    auth_spec_signcalc_test_data.yaml auth_spec.py policy.txt framework.py \
    jclient.py jcloud.py s3cmd.py jclient_spec.py jcloud_spec.py \
    s3cmd_spec.py pathstyle.s3cfg virtualhoststyle.s3cfg rollback_spec.py \
    negative_spec.py shutdown_spec.py s3fi.py s3kvs.py s3kvstool.py \
    s3client_config.py cloviskvscli.sh prechecksystest.sh cloud_setup.py \
    mmcloud.py mmcloud_spec.py runallsystest.sh setup.sh fs_helpers.py cfg \
    tools resources"
for file in $test_files
do
    cp -r "$SRC_ROOT/st/clitests/$file" "$TEST_DIR/"
done

# create tar file
tar -cvf s3-specs.tar $TARGET_DIR

# remove dir
rm -rf $TARGET_DIR
