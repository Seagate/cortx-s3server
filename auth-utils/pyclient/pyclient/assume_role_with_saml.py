class AssumeRoleWithSAML:
    def __init__(self, sts_client, cli_args):
        self.sts_client = sts_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.saml_principal_arn is None):
            print("SAML principal ARN is required.")
            return

        if(self.cli_args.saml_role_arn is None):
            print("SAML role ARN is required.")
            return

        if(self.cli_args.saml_assertion is None):
            print("SAML assertion is required.")
            return

        file_path = os.path.abspath(self.cli_args.saml_assertion)
        if(not os.path.isfile(file_path)):
            print("Saml assertion file not found.")
            return

        with open (file_path, "r") as saml_assertion_file:
            assertion = federation_token_file.read()

        assume_role_with_saml_args = {}
        assume_role_with_saml_args['SAMLAssertion'] = assertion
        assume_role_with_saml_args['RoleArn'] = self.cli_args.saml_role_arn
        assume_role_with_saml_args['PrincipalArn'] = self.cli_args.saml_principal_arn

        if(not self.cli_args.duration is None):
            assume_role_with_saml_args['DurationSeconds'] = self.cli_args.duration

        if(not self.cli_args.file is None):
            file_path = os.path.abspath(self.cli_args.file)
            if(not os.path.isfile(file_path)):
                print("Policy file not found.")
                return

            with open (file_path, "r") as federation_token_file:
                policy = federation_token_file.read()

            assume_role_with_saml_args['Policy'] = policy

        try:
            result = self.sts_client.assume_role_with_saml(**assume_role_with_saml_args)
        except Exception as ex:
            print("Exception occured while creating credentials with saml.")
            print(str(ex))
            return

        print("FederatedUserId- %s, AccessKeyId- %s, SecretAccessKey- %s, SessionToken- %s" % (
                result['FederatedUser']['FederatedUserId'],
                result['Credentials']['AccessKeyId'],
                result['Credentials']['SecretAccessKey'],
                result['Credentials']['SessionToken']))
