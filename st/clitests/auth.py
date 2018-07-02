import os
from framework import PyCliTest
from framework import Config
from framework import logit
from s3client_config import S3ClientConfig

class AuthTest(PyCliTest):
    def __init__(self, description):
        self.test_data_dir = os.path.join(os.path.dirname(__file__), "test_data")
        super(AuthTest, self).__init__(description)

    def setup(self):
        super(AuthTest, self).setup()

    def run(self, cmd_args = None):
        super(AuthTest, self).run(cmd_args)

    def teardown(self):
        super(AuthTest, self).teardown()

    def with_cli(self, cmd):
        if Config.no_ssl and 's3iamcli' in cmd:
            cmd = cmd + ' --no-ssl'
        super(AuthTest, self).with_cli(cmd)

    def create_account(self, **account_args):
        cmd = "s3iamcli createaccount -n %s -e %s --ldapuser %s --ldappasswd %s" % (
                 account_args['AccountName'], account_args['Email'], account_args['ldapuser'], account_args['ldappasswd'])

        self.with_cli(cmd)
        return self

    def list_account(self, **account_args):
        if ('ldapuser' in account_args) and 'ldappasswd' in account_args:
            if (account_args['ldapuser'] != None) and (account_args['ldappasswd'] != None):
                cmd = "s3iamcli listaccounts --ldapuser %s --ldappasswd %s" % (account_args['ldapuser'], account_args['ldappasswd'])
        else:
            cmd = "s3iamcli listaccounts"

        self.with_cli(cmd)
        return self

    def delete_account(self, **account_args):
        cmd = "s3iamcli deleteaccount -n %s --access_key '%s' --secret_key '%s'" % (
                 account_args['AccountName'], S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key)

        if ('force' in account_args.keys() and account_args['force']):
            cmd += " --force"

        self.with_cli(cmd)
        return self

    def create_user(self, **user_args):
        cmd = "s3iamcli createuser --access_key '%s' --secret_key '%s' -n %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, user_args['UserName'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('Path' in user_args.keys()):
            cmd += " -p %s" % user_args['Path']

        self.with_cli(cmd)
        return self

    def update_user(self, **user_args):
        cmd = "s3iamcli updateuser --access_key '%s' --secret_key '%s' -n %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, user_args['UserName'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('NewUserName' in user_args.keys()):
            cmd += " --new_user %s" % user_args['NewUserName']

        if('NewPath' in user_args.keys()):
            cmd += " -p %s" % user_args['NewPath']

        self.with_cli(cmd)
        return self

    def delete_user(self, **user_args):
        cmd = "s3iamcli deleteuser --access_key '%s' --secret_key '%s' -n %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, user_args['UserName'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def list_users(self, **user_args):
        if (S3ClientConfig.access_key_id != None) and \
           (S3ClientConfig.secret_key != None):
            cmd = "s3iamcli listusers --access_key '%s' --secret_key '%s'" % (
                     S3ClientConfig.access_key_id,
                     S3ClientConfig.secret_key)
        else:
            cmd = "s3iamcli listusers"

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('PathPrefix' in user_args.keys()):
            cmd += " -p %s" % user_args['PathPrefix']

        self.with_cli(cmd)
        return self

    def create_access_key(self, **access_key_args):
        cmd = "s3iamcli createaccesskey --access_key '%s' --secret_key '%s' " % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def delete_access_key(self, **access_key_args):
        cmd = "s3iamcli deleteaccesskey --access_key '%s' --secret_key '%s' -k %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, access_key_args['AccessKeyId'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def update_access_key(self, **access_key_args):
        cmd = "s3iamcli updateaccesskey --access_key '%s' --secret_key '%s' -k %s -s %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, access_key_args['AccessKeyId'],
                access_key_args['Status'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def list_access_keys(self, **access_key_args):
        cmd = "s3iamcli listaccesskeys --access_key '%s' --secret_key '%s'" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def create_role(self, **role_args):
        cmd = "s3iamcli createrole --access_key '%s' --secret_key '%s' -n %s -f %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, role_args['RoleName'],
                 role_args['AssumeRolePolicyDocument'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('Path' in role_args.keys()):
            cmd += " -p %s" % role_args['Path']

        self.with_cli(cmd)
        return self

    def delete_role(self, **role_args):
        cmd = "s3iamcli deleterole --access_key '%s' --secret_key '%s' -n %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, role_args['RoleName'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def list_roles(self, **role_args):
        cmd = "s3iamcli listroles --access_key '%s' --secret_key '%s'" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('Path' in role_args.keys()):
            cmd += " -p %s" % role_args['Path']

        self.with_cli(cmd)
        return self

    def create_saml_provider(self, **saml_provider_args):
        cmd = "s3iamcli createsamlprovider --access_key '%s' --secret_key '%s' -n %s -f %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, saml_provider_args['Name'],
                 saml_provider_args['SAMLMetadataDocument'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def update_saml_provider(self, **saml_provider_args):
        cmd = "s3iamcli updatesamlprovider --access_key '%s' --secret_key '%s' \
                --arn '%s' -f %s" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, saml_provider_args['SAMLProviderArn'],
                 saml_provider_args['SAMLMetadataDocument'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def list_saml_providers(self, **saml_provider_args):
        cmd = "s3iamcli listsamlproviders --access_key '%s' --secret_key '%s'" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def delete_saml_provider(self, **saml_provider_args):
        cmd = "s3iamcli deletesamlprovider --access_key '%s' --secret_key '%s' \
                --arn '%s'" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, saml_provider_args['SAMLProviderArn'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def get_federation_token(self, **federation_token_args):
        cmd = "s3iamcli getfederationtoken --access_key '%s' --secret_key '%s' \
                -n '%s'" % (
                 S3ClientConfig.access_key_id,
                 S3ClientConfig.secret_key, federation_token_args['Name'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('Policy' in federation_token_args.keys()):
            cmd += " -f %s" % federation_token_args['Policy']

        if('DurationSeconds' in federation_token_args.keys()):
            cmd += " --duration %s" % federation_token_args['DurationSeconds']

        self.with_cli(cmd)
        return self

    def inject_fault(self, fault_point, mode, value):
        cmd = "curl -s --data 'Action=InjectFault&FaultPoint=%s&Mode=%s&Value=%s' %s"\
                % (fault_point, mode, value, S3ClientConfig.iam_uri)

        self.with_cli(cmd)
        return self

    def reset_fault(self, fault_point):
        cmd = "curl -s --data 'Action=ResetFault&FaultPoint=%s' %s"\
                % (fault_point, S3ClientConfig.iam_uri)

        self.with_cli(cmd)
        return self
