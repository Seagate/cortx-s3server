import os

class SAMLProvider:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.file is None):
            print("SAML Metadata Document is required to create role")
            return

        if(self.cli_args.name is None):
            print("Provider name is required to create SAML provider.")
            return

        file_path = os.path.abspath(self.cli_args.file)
        if(not os.path.isfile(file_path)):
            print("File not found.")
            return

        with open (file_path, "r") as saml_metadata_file:
            saml_metadata = saml_metadata_file.read()

        saml_provider_args = {}
        saml_provider_args['SAMLMetadataDocument'] = saml_metadata
        saml_provider_args['Name'] = self.cli_args.name

        try:
            result = self.iam_client.create_saml_provider(**saml_provider_args)
        except Exception as ex:
            print("Exception occured while creating saml provider.")
            print(str(ex))
            return

        print("SAMLProviderArn = %s" % (result['SAMLProviderArn']))

    def delete(self):
        if(self.cli_args.arn is None):
            print("SAML Provider ARN is required to delete.")
            return

        saml_provider_args = {}
        saml_provider_args['SAMLProviderArn'] = self.cli_args.arn

        try:
            self.iam_client.delete_saml_provider(**saml_provider_args)
        except Exception as ex:
            print("Exception occured while deleting SAML provider.")
            print(str(ex))
            return

        print("SAML provider deleted.")

    def update(self):
        if(self.cli_args.file is None):
            print("SAML Metadata Document is required to update saml provider.")
            return

        if(self.cli_args.arn is None):
            print("SAML Provider ARN is required to update saml provider.")
            return

        file_path = os.path.abspath(self.cli_args.file)
        if(not os.path.isfile(file_path)):
            print("File not found.")
            return

        with open (file_path, "r") as saml_metadata_file:
            saml_metadata = saml_metadata_file.read()

        saml_provider_args = {}
        saml_provider_args['SAMLMetadataDocument'] = saml_metadata
        saml_provider_args['SAMLProviderArn'] = self.cli_args.arn

        try:
            self.iam_client.update_saml_provider(**saml_provider_args)
        except Exception as ex:
            print("Exception occured while updating SAML provider.")
            print(str(ex))
            return

        print("SAML provider Updated.")

    def list(self):
        try:
            result = self.iam_client.list_saml_providers()
        except Exception as ex:
            print("Exception occured while listing SAML providers.")
            print(str(ex))
            return

        for provider in result['SAMLProviderList']:
            print("ARN = %s, ValidUntil = %s" % (provider['Arn'], provider['ValidUntil']))
