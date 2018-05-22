import os

class Role:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.file is None):
            print("Assume role policy document is required to create role.")
            return

        if(self.cli_args.name is None):
            print("RoleName is required to create role.")
            return

        file_path = os.path.abspath(self.cli_args.file)
        if(not os.path.isfile(file_path)):
            print("File not found.")
            return

        with open (file_path, "r") as assume_role_file:
            assume_role_policy = assume_role_file.read()

        role_args = {}
        role_args['AssumeRolePolicyDocument'] = assume_role_policy
        role_args['RoleName'] = self.cli_args.name

        if(not self.cli_args.path is None):
            role_args['Path'] = self.cli_args.path

        try:
            result = self.iam_client.create_role(**role_args)
        except Exception as ex:
            print("Exception occured while creating role.")
            print(str(ex))
            return

        print("RoleId = %s, RoleName = %s, ARN = %s, Path = %s" % (result['Role']['RoleId'],
                    result['Role']['RoleName'], result['Role']['Arn'], result['Role']['Path']))

    def delete(self):
        if(self.cli_args.name is None):
            print("Role name is required to delete role.")
            return

        role_args = {}
        role_args['RoleName'] = self.cli_args.name

        try:
            self.iam_client.delete_role(**role_args)
        except Exception as ex:
            print("Exception occured while deleting role.")
            print(str(ex))
            return

        print("Role deleted.")

    def list(self):
        role_args = {}
        if(not self.cli_args.path is None):
            role_args['PathPrefix'] = self.cli_args.path

        try:
            result = self.iam_client.list_roles(**role_args)
        except Exception as ex:
            print("Exception occured while listing roles.")
            print(str(ex))
            return

        for role in result['Roles']:
            print("RoleId = %s, RoleName = %s, ARN = %s, Path = %s" % (role['RoleId'],
                        role['RoleName'], role['Arn'], role['Path']))
