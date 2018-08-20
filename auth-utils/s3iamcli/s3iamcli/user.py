class User:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.name is None):
            print("User name is required for user creation")
            return

        user_args = {}
        user_args['UserName'] = self.cli_args.name

        if(not self.cli_args.path is None):
            user_args['Path'] = self.cli_args.path

        try:
            result = self.iam_client.create_user(**user_args)
        except Exception as ex:
            print("Failed to create user.")
            print(str(ex))
            return

        print("UserId = %s, ARN = %s, Path = %s" % (result['User']['UserId'],
                    result['User']['Arn'], result['User']['Path']))

    def delete(self):
        if(self.cli_args.name is None):
            print("User name is required to delete user.")
            return

        user_args = {}
        user_args['UserName'] = self.cli_args.name

        try:
            self.iam_client.delete_user(**user_args)
        except Exception as ex:
            print("Failed to delete user.")
            print(str(ex))
            return

        print("User deleted.")

    def update(self):
        if(self.cli_args.name is None):
            print("User name is required to update user.")
            return

        user_args = {}
        user_args['UserName'] = self.cli_args.name

        if(not self.cli_args.path is None):
            user_args['NewPath'] = self.cli_args.path

        if(not self.cli_args.new_user is None):
            user_args['NewUserName'] = self.cli_args.new_user

        try:
            self.iam_client.update_user(**user_args)
        except Exception as ex:
            print("Failed to update user info.")
            print(str(ex))
            return

        print("User Updated.")

    def list(self):
        user_args = {}
        if(not self.cli_args.path is None):
            user_args['PathPrefix'] = self.cli_args.path

        try:
            result = self.iam_client.list_users(**user_args)
        except Exception as ex:
            print("Failed to list users.")
            print(str(ex))
            return

        for user in result['Users']:
            print("UserId = %s, UserName = %s, ARN = %s, Path = %s" % (user['UserId'],
                        user['UserName'], user['Arn'], user['Path']))
