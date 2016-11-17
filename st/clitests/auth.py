import os
from framework import PyCliTest
from framework import Config
from framework import logit
from s3client_config import S3ClientConfig

class AuthTest(PyCliTest):
    def __init__(self, description):
        self.tools_dir = os.path.join(os.path.dirname(__file__), "tools")
        super(AuthTest, self).__init__(description)

    def setup(self):
        super(AuthTest, self).setup()

    def run(self):
        super(AuthTest, self).run()

    def teardown(self):
        super(AuthTest, self).teardown()

    def get_pyclient_dir(self):
        return os.path.join(os.path.dirname(__file__), '../../', 'auth-utils', 'pyclient', 'pyclient')

    def create_account(self, **account_args):
        self.with_cli("python %s createaccount -n %s -e %s" % (self.get_pyclient_dir(),
                        account_args['AccountName'], account_args['Email']))
        return self

    def list_account(self):
        self.with_cli("python %s listaccounts" % (self.get_pyclient_dir()))
        return self

    def create_user(self, **user_args):
        cmd = "python %s createuser --access_key '%s' --secret_key '%s' -n %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, user_args['UserName'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('Path' in user_args.keys()):
            cmd += " -p %s" % user_args['Path']

        self.with_cli(cmd)
        return self

    def update_user(self, **user_args):
        cmd = "python %s updateuser --access_key '%s' --secret_key '%s' -n %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
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
        cmd = "python %s deleteuser --access_key '%s' --secret_key '%s' -n %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, user_args['UserName'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def list_users(self, **user_args):
        cmd = "python %s listusers --access_key '%s' --secret_key '%s'" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('PathPrefix' in user_args.keys()):
            cmd += " -p %s" % user_args['PathPrefix']

        self.with_cli(cmd)
        return self

    def create_access_key(self, **access_key_args):
        cmd = "python %s createaccesskey --access_key '%s' --secret_key '%s' " % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def delete_access_key(self, **access_key_args):
        cmd = "python %s deleteaccesskey --access_key '%s' --secret_key '%s' -k %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, access_key_args['AccessKeyId'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def update_access_key(self, **access_key_args):
        cmd = "python %s updateaccesskey --access_key '%s' --secret_key '%s' -k %s -s %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, access_key_args['AccessKeyId'],
                access_key_args['Status'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def list_access_keys(self, **access_key_args):
        cmd = "python %s listaccesskeys --access_key '%s' --secret_key '%s'" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('UserName' in access_key_args.keys()):
            cmd += " -n %s" % access_key_args['UserName']

        self.with_cli(cmd)
        return self

    def create_role(self, **role_args):
        cmd = "python %s createrole --access_key '%s' --secret_key '%s' -n %s -f %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, role_args['RoleName'],
                role_args['AssumeRolePolicyDocument'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('Path' in role_args.keys()):
            cmd += " -p %s" % role_args['Path']

        self.with_cli(cmd)
        return self

    def delete_role(self, **role_args):
        cmd = "python %s deleterole --access_key '%s' --secret_key '%s' -n %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, role_args['RoleName'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def list_roles(self, **role_args):
        cmd = "python %s listroles --access_key '%s' --secret_key '%s'" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        if('Path' in role_args.keys()):
            cmd += " -p %s" % role_args['Path']

        self.with_cli(cmd)
        return self

    def create_saml_provider(self, **saml_provider_args):
        cmd = "python %s createsamlprovider --access_key '%s' --secret_key '%s' -n %s -f %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, saml_provider_args['Name'],
                saml_provider_args['SAMLMetadataDocument'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def update_saml_provider(self, **saml_provider_args):
        cmd = "python %s updatesamlprovider --access_key '%s' --secret_key '%s' \
                --arn '%s' -f %s" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, saml_provider_args['SAMLProviderArn'],
                saml_provider_args['SAMLMetadataDocument'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def list_saml_providers(self, **saml_provider_args):
        cmd = "python %s listsamlproviders --access_key '%s' --secret_key '%s'" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key)

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def delete_saml_provider(self, **saml_provider_args):
        cmd = "python %s deletesamlprovider --access_key '%s' --secret_key '%s' \
                --arn '%s'" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
                S3ClientConfig.secret_key, saml_provider_args['SAMLProviderArn'])

        if(not S3ClientConfig.token is ""):
            cmd += " --session_token '%s'" % S3ClientConfig.token

        self.with_cli(cmd)
        return self

    def get_federation_token(self, **federation_token_args):
        cmd = "python %s getfederationtoken --access_key '%s' --secret_key '%s' \
                -n '%s'" % (
                self.get_pyclient_dir(), S3ClientConfig.access_key_id,
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
