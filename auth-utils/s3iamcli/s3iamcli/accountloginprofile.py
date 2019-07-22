import http.client, urllib.parse
import sys
import datetime
from s3iamcli.util import sign_request_v2
from s3iamcli.util import sign_request_v4
from s3iamcli.util import get_timestamp
from s3iamcli.conn_manager import ConnMan
from s3iamcli.error_response import ErrorResponse
from s3iamcli.create_accountloginprofile_response import CreateAccountLoginProfileResponse
from s3iamcli.get_accountloginprofile_response import GetAccountLoginProfileResponse
from s3iamcli.list_account_response import ListAccountResponse
from s3iamcli.reset_key_response import ResetAccountAccessKey
from s3iamcli.config import Config

class AccountLoginProfile:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    # list all accounts
    def list(self):
        # Get host value from url https://iam.seagate.com:9443
        if(self.cli_args.name is None):
            print("Account name is required.")
            return

        url_parse_result  = urllib.parse.urlparse(Config.endpoint)

        epoch_t = datetime.datetime.utcnow();

        body = urllib.parse.urlencode({'Action' : 'GetAccountLoginProfile','AccountName' : self.cli_args.name})
        headers = {'content-type': 'application/x-www-form-urlencoded',
                'Accept': 'text/plain'}
        headers['Authorization'] = sign_request_v4('POST', '/', body, epoch_t,
            url_parse_result.netloc, Config.service, Config.default_region);
        headers['X-Amz-Date'] = get_timestamp(epoch_t);

        if(headers['Authorization'] is None):
            print("Failed to generate v4 signature")
            sys.exit(1)
        response = ConnMan.send_post_request(body, headers)
        if response['status'] == 200:
            accounts = GetAccountLoginProfileResponse(response)

            if accounts.is_valid_response():
                accounts.print_account_login_profile_info()
            else:
                # unlikely message corruption case in network
                print("Failed to list login profile")
                sys.exit(0)
        elif(response['status'] == 503):
            print("Failed to get login profile")
            print("An error occurred (503) when calling the GetAccountLoginProfile operation : " + response['reason'])
        else:
            print("Failed to get login profile")
            error = ErrorResponse(response)
            error_message = error.get_error_message()
            print(error_message)

    def create(self):
        if(self.cli_args.name is None or self.cli_args.name is ''):
            print("Account name is required")
            return

        if(self.cli_args.password is None):
            print("Account login password is required")
            return

        passwordResetRequired = False;
        if(self.cli_args.password_reset_required):
            passwordResetRequired = True

        # Get host value from url https://iam.seagate.com:9443
        url_parse_result  = urllib.parse.urlparse(Config.endpoint)
        epoch_t = datetime.datetime.utcnow();
        body = urllib.parse.urlencode({'Action' : 'CreateAccountLoginProfile',
            'AccountName' : self.cli_args.name, 'Password' : self.cli_args.password,
            'PasswordResetRequired' : passwordResetRequired})
        headers = {'content-type': 'application/x-www-form-urlencoded',
                'Accept': 'text/plain'}
        headers['Authorization'] = sign_request_v4('POST', '/', body, epoch_t, url_parse_result.netloc,
            Config.service, Config.default_region);
        headers['X-Amz-Date'] = get_timestamp(epoch_t);
        if(headers['Authorization'] is None):
            print("Failed to generate v4 signature")
            sys.exit(1)
        result = ConnMan.send_post_request(body, headers)
        # Validate response
        if(result['status'] == 201):
            profile = CreateAccountLoginProfileResponse(result)

            if profile.is_valid_response():
                profile.print_profile_info()
            else:
                # unlikely message corruption case in network
                print("Account login profile was created. For details try get account login profile.")
                sys.exit(0)
        elif(result['status'] == 503):
            print("Account login profile wasn't created.")
            print("An error occurred (503) when calling the CreateAccountLoginProfile operation : " + result['reason'])
        else:
            print("Account login profile wasn't created.")
            error = ErrorResponse(result)
            error_message = error.get_error_message()
            print(error_message)

    def update(self):
        if(self.cli_args.name is None):
            print("Account name is required for UpdateAccountLoginProfile")
            return

        passwordResetRequired = False
        if(self.cli_args.password_reset_required):
            passwordResetRequired = True
        if(self.cli_args.password is None) and (self.cli_args.password_reset_required is False) and (self.cli_args.no_password_reset_required is False):
            print("Please provide password or password-reset flag")
            return

        # Get host value from url https://iam.seagate.com:9443
        url_parse_result  = urllib.parse.urlparse(Config.endpoint)
        epoch_t = datetime.datetime.utcnow();
        if(self.cli_args.password is None):
            body = urllib.parse.urlencode({'Action' : 'UpdateAccountLoginProfile',
                'AccountName' : self.cli_args.name, 'PasswordResetRequired' : passwordResetRequired})
        else:
            body = urllib.parse.urlencode({'Action' : 'UpdateAccountLoginProfile',
                'AccountName' : self.cli_args.name, 'Password' : self.cli_args.password,
                'PasswordResetRequired' : passwordResetRequired})
        headers = {'content-type': 'application/x-www-form-urlencoded',
                'Accept': 'text/plain'}
        headers['Authorization'] = sign_request_v4('POST', '/', body, epoch_t, url_parse_result.netloc,
            Config.service, Config.default_region);
        headers['X-Amz-Date'] = get_timestamp(epoch_t);
        if(headers['Authorization'] is None):
            print("Failed to generate v4 signature")
            sys.exit(1)
        result = ConnMan.send_post_request(body, headers)
        # Validate response
        if(result['status'] == 200):
            print("Account login profile updated.")
            sys.exit(0)
        elif(result['status'] == 503):
            print("Account login profile wasn't Updated.")
            print("An error occurred (503) when calling the UpdateAccountLoginProfile operation : " + result['reason'])
        else:
            print("Account login profile wasn't Updated.")
            error = ErrorResponse(result)
            error_message = error.get_error_message()
            print(error_message)

