import os

class FederationToken:
    def __init__(self, sts_client, cli_args):
        self.sts_client = sts_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.name is None):
            print("Name is required to create federation token.")
            return

        federation_token_args = {}
        federation_token_args['Name'] = self.cli_args.name

        if(not self.cli_args.duration is None):
            federation_token_args['DurationSeconds'] = self.cli_args.duration

        if(not self.cli_args.file is None):
            file_path = os.path.abspath(self.cli_args.file)
            if(not os.path.isfile(file_path)):
                print("Policy file not found.")
                return

            with open (file_path, "r") as federation_token_file:
                policy = federation_token_file.read()

            federation_token_args['Policy'] = policy

        try:
            result = self.sts_client.get_federation_token(**federation_token_args)
        except Exception as ex:
            print("Exception occured while creating federation token.")
            print(str(ex))
            return

        print("FederatedUserId = %s, AccessKeyId = %s, SecretAccessKey = %s, SessionToken = %s" % (
                result['FederatedUser']['FederatedUserId'],
                result['Credentials']['AccessKeyId'],
                result['Credentials']['SecretAccessKey'],
                result['Credentials']['SessionToken']))
