class Group:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        policy_args = {}
        if(self.cli_args.name is None):
            print("Group name is required.")
            return

        policy_args['GroupName'] = self.cli_args.name

        if(not self.cli_args.path is None):
            policy_args['Path'] = self.cli_args.path

        try:
            result = self.iam_client.create_group(**policy_args)
        except Exception as ex:
            print("Exception occured while creating group.")
            print(str(ex))
            return

        print("GroupId = %s, GroupName = %s, Arn = %s" % (
                result['Group']['GroupId'],
                result['Group']['GroupName'],
                result['Group']['Arn'],
                ))
