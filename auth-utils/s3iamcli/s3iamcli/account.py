import http.client, urllib.parse
import json
import xmltodict
from s3iamcli.util import sign_request_v2
from s3iamcli.conn_manager import ConnMan

class Account:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.name is None):
            print("Account name is required.")
            return

        if(self.cli_args.email is None):
            print("Email Id of the user is required to create an Account")
            return

        body = urllib.parse.urlencode({'Action' : 'CreateAccount',
            'AccountName' : self.cli_args.name, 'Email' : self.cli_args.email})
        headers = {'content-type': 'application/x-www-form-urlencoded',
                'Accept': 'text/plain'}
        headers['Authorization'] = sign_request_v2('POST', '/', {}, headers)
        response = ConnMan.send_post_request(body, headers)
        if(response['status'] == 201):
            account_response = json.loads(json.dumps(xmltodict.parse(response['body'])))
            account = account_response['CreateAccountResponse']['CreateAccountResult']['Account']
            print("AccountId = %s, CanonicalId = %s, RootUserName = %s, AccessKeyId = %s, SecretKey = %s" %
                    (account['AccountId'], account['CanonicalId'], account['RootUserName'],
                        account['AccessKeyId'], account['RootSecretKeyId']))
        else:
            print("Account wasn't created.")
            print(response['reason'])

    def _print_account(self, account):
        print("AccountName = %s, AccountId = %s, CanonicalId = %s, Email = %s"
                % (account['AccountName'], account['AccountId'], account['CanonicalId'], account['Email']));

    # list all accounts
    def list(self):
        body = urllib.parse.urlencode({'Action' : 'ListAccounts'})
        headers = {'content-type': 'application/x-www-form-urlencoded',
                'Accept': 'text/plain'}
        headers['Authorization'] = sign_request_v2('POST', '/', {}, headers)
        response = ConnMan.send_post_request(body, headers)

        if response['status'] == 200:
            accounts_response = json.loads(json.dumps(xmltodict.parse(response['body'])))
            accounts = accounts_response['ListAccountsResponse']['ListAccountsResult']['Accounts']
            if accounts is None:
                print("No accounts found.")
                return
            account_members = accounts['member']
            # For only one account, account_members will be of type dict, convert it to list type
            if type(account_members) == dict:
                self._print_account(account_members)
            else:
                for account in account_members:
                    self._print_account(account)
        else:
            print("Failed to list accounts!")
            print(response['reason'])

    # delete account
    def delete(self):
        if(self.cli_args.name is None):
            print('Account name is required.')
            return

        headers = {'content-type': 'application/x-www-form-urlencoded',
                'Accept': 'text/plain'}
        headers['Authorization'] = sign_request_v2('POST', '/', {}, headers)

        account_params = {'Action': 'DeleteAccount', 'AccountName': self.cli_args.name}
        if self.cli_args.force:
            account_params['force'] = True

        body = urllib.parse.urlencode(account_params)
        response = ConnMan.send_post_request(body, headers)

        if response['status'] == 200:
            print('Account deleted successfully.')
        else:
            print('Account cannot be deleted.')
            print(response['body'])
