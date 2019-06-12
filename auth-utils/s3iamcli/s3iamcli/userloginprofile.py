class UserLoginProfile:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.name is None):
            print("User name is required for user login-profile creation")
            return

        if(self.cli_args.password is None):
            print("User password is required for user login-profile creation")
            return
        user_args = {}
        user_args['UserName'] = self.cli_args.name
        user_args['Password'] = self.cli_args.password
        user_args['PasswordResetRequired'] = False
        if(self.cli_args.password_reset_required):
            user_args['PasswordResetRequired'] = True
        try:
            result = self.iam_client.create_login_profile(**user_args)
        except Exception as ex:
            print("Failed to create userloginprofile.")
            print(str(ex))
            return
        profile = (result['LoginProfile'])
        print("Login Profile %s %s %s" % (profile['CreateDate'], profile['PasswordResetRequired'], profile['UserName']))

    def get(self):
        if(self.cli_args.name is None):
            print("User name is required for getting Login Profile")
            return

        user_args = {}
        user_args['UserName'] = self.cli_args.name
        try:
            result = self.iam_client.get_login_profile(**user_args)
        except Exception as ex:
            print("Failed to get Login Profile for "+ user_args['UserName'])
            print(str(ex))
            return
        profile = (result['LoginProfile'])
        print("Login Profile %s %s %s" % (profile['CreateDate'], profile['PasswordResetRequired'], profile['UserName']))
