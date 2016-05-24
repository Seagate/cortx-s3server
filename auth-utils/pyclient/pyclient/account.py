import http.client, urllib.parse
import json
import xmltodict

class Account:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.name is None):
            print("Account name is required for Account creation")
            return

        if(self.cli_args.email is None):
            print("Email Id of the user is required to create an Account")
            return

        try:
            auth_endpoint = self.iam_client._endpoint.host
        except Exception as ex:
            print("Exception occured while getting end point")
            print(str(ex))
            return

        auth_server_name = urllib.parse.urlparse(auth_endpoint).netloc
        account_params = urllib.parse.urlencode({'Action' : 'CreateAccount',
                                           'AccountName' : self.cli_args.name,
                                           'Email' : self.cli_args.email})

        headers = {"Content-type": "application/x-www-form-urlencoded",
                    "Accept": "text/plain"}

        conn = http.client.HTTPConnection(auth_server_name)
        conn.request("POST", "/", account_params, headers)
        response = conn.getresponse()
        data = response.read()
        conn.close()

        if(response.status == 201):
            account_response = json.loads(json.dumps(xmltodict.parse(data)))
            account = account_response['CreateAccountResponse']['CreateAccountResult']['Account']

            print("AccountId = %s, CanonicalId = %s, RootUserName = %s, AccessKeyId = %s, SecretKey = %s" %
                    (account['AccountId'], account['CanonicalId'], account['RootUserName'], account['AccessKeyId'], account['RootSecretKeyId']))
        else:
            print("Account wasn't created.")
            print(response.reason)
