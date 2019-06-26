import logging
from collections import OrderedDict
from s3iamcli.authserver_response import AuthServerResponse

class GetAccountLoginProfileResponse(AuthServerResponse):

    # Printing account info while listing users.
    def print_account_login_profile_info(self):

        if self.accountloginprofile is None:
            print("No account login profile found.")
            return
        print("Account Login Profile: CreateDate = %s, PasswordResetRequired = %s, AccountName = %s" %
              (self.get_value(self.accountloginprofile, 'CreateDate'),
               self.get_value(self.accountloginprofile, 'PasswordResetRequired'),
               self.get_value(self.accountloginprofile, 'AccountName')))

    # Validator for list account operation
    def validate_response(self):
        super(GetAccountLoginProfileResponse, self).validate_response()
        self.accountloginprofile = None
        try:
            self.accountloginprofile = (self.response_dict['GetAccountLoginProfileResponse']['GetAccountLoginProfileResult']
                             ['LoginProfile'])
        except KeyError:
            self.is_valid = False
            logging.exception('Failed to list account login profile from account login profile response')

