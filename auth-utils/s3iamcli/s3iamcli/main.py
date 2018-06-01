import argparse
import re
import yaml
import sys
import os
import imp
import botocore
import logging
import shutil

from boto3.session import Session
from s3iamcli.config import Credentials
from s3iamcli.config import Config
from s3iamcli.account import Account

class S3IamCli:
    def iam_usage(self):
        return '''
        CreateAccount -n <Account Name> -e <Email Id>
        ListAccounts
        CreateAccessKey
            -n <User Name>
        CreateUser -n <User Name>
            -p <Path (Optional)
        DeleteAccount -n <Account Name> -r <true|false>
        DeleteAccesskey -k <Access Key Id to be deleted>
            -n <User Name>
        DeleteUser -n <User Name>
        ListAccessKeys
            -n <User Name>
        ListUsers
            -p <Path Prefix>
        UpdateUser -n <Old User Name>
            --new_user <New User Name> -p <New Path>
        UpdateAccessKey -k <access key to be updated> -s <Active/Inactive>
            -n <User Name>
        '''
    def iam_usage_hidden(self):
        return '''

        -----------------------------------
        *** hidden_help options ****
        -----------------------------------

        AssumeRoleWithSAML --saml_principal_arn <SAML IDP ARN> --saml_role_arn <Role ARN>
        --saml_assertion <File containing SAML Assertion>
            -f <Policy Document> -d <Duration in seconds>
        CreateGroup -n <Group Name>
            -p <Path>
        CreatePolicy -n <Policy Name> -f <Path of the policy document>
            -p <Path> --description <Description of the policy>
        CreateRole -n <Role Name> -f <Path of role policy document>
            -p <Path>
        CreateSAMLProvider -n <Name of saml provider> -f <Path of metadata file>
        Delete Role -n <Role Name>
        DeleteSamlProvider --arn <Saml Provider ARN>
        GetFederationToken -n <User Name>
            -d <Duration in second> -f <Policiy Document File>
        ListRoles -p <Path Prefix>
        ListSamlProviders
        UpdateSAMLProvider --arn <SAML Provider ARN> -f <Path of metadata file>
        '''

    def get_conf_dir(self):
        return os.path.join(os.path.dirname(__file__),'config')

    def load_config(self, cli_args):
        conf_home_dir = os.path.join(os.path.expanduser('~'), '.sgs3iamcli')
        conf_file = os.path.join(conf_home_dir,'config.yaml')
        if not os.path.isfile(conf_file):
            try:
               os.stat(conf_home_dir)
            except:
               os.mkdir(conf_home_dir)
            shutil.copy(os.path.join(self.get_conf_dir(),'s3iamclicfg.yaml'), conf_file)

        if not os.access(conf_file, os.R_OK):
            print("Failed to read " + conf_file + " it doesn't have read access")
            sys.exit()
        with open(conf_file, 'r') as f:
            config = yaml.safe_load(f)

        service = Config.service.upper()

        if cli_args.use_ssl:
            Config.use_ssl = True
            service = service + '_HTTPS'
            Config.ca_cert_file = config['SSL']['CA_CERT_FILE']
            if('~' == Config.ca_cert_file[0]):
               Config.ca_cert_file = os.path.expanduser('~') + Config.ca_cert_file[1:len(Config.ca_cert_file)]

            Config.check_ssl_hostname = config['SSL']['CHECK_SSL_HOSTNAME']
            Config.verify_ssl_cert = config['SSL']['VERIFY_SSL_CERT']

        if Config.endpoint != None:
            Config.endpoint = config['ENDPOINTS'][service]

        Credentials.access_key = cli_args.access_key
        Credentials.secret_key = cli_args.secret_key

        if config['BOTO']['ENABLE_LOGGER']:
            logging.basicConfig(filename=config['BOTO']['LOG_FILE_PATH'],
                level=config['BOTO']['LOG_LEVEL'])

        if 'DEFAULT_REGION' in config and config['DEFAULT_REGION']:
            Config.default_region = config['DEFAULT_REGION']
        else:
            Config.default_region = 'us-west2'

    def load_controller_action(self):
        controller_action_file = os.path.join(self.get_conf_dir(), 'controller_action.yaml')
        with open(controller_action_file, 'r') as f:
            return yaml.safe_load(f)

     # Import module
    def import_module(self, module_name):
        fp, pathname, description = imp.find_module(module_name, [os.path.dirname(__file__)]) #, sys.path)

        try:
            return imp.load_module(module_name, fp, pathname, description)
        finally:
            if fp:
                fp.close()

    # Convert the string to Class Object.
    def str_to_class(self, module, class_name):
        return getattr(module, class_name)

    # Create a new IAM serssion.
    def get_session(self, access_key, secret_key, session_token = None):
        return Session(aws_access_key_id=access_key,
                      aws_secret_access_key=secret_key,
                      aws_session_token=session_token)

    # Create an IAM client.
    def get_client(self, session):
        if Config.use_ssl:
            if Config.verify_ssl_cert:
                return session.client(Config.service, endpoint_url=Config.endpoint, verify=Config.ca_cert_file, region_name=Config.default_region)
            return session.client(Config.service, endpoint_url=Config.endpoint, verify=False, region_name=Config.default_region)
        else:
            return session.client(Config.service, endpoint_url=Config.endpoint, use_ssl=False, region_name=Config.default_region)

    # run method
    def run(self):
        parser = argparse.ArgumentParser(usage = self.iam_usage())
        parser.add_argument("action", help="Action to be performed.")
        parser.add_argument("-n", "--name", help="Name.")
        parser.add_argument("-e", "--email", help="Email id.")
        parser.add_argument("-p", "--path", help="Path or Path Prefix.")
        parser.add_argument("-f", "--file", help="File Path.")
        parser.add_argument("-d", "--duration", help="Access Key Duration.", type = int)
        parser.add_argument("-k", "--access_key_update", help="Access Key to be updated or deleted.")
        parser.add_argument("-s", "--status", help="Active/Inactive")
        parser.add_argument("--force", help="Delete account forcefully.", action='store_true')
        parser.add_argument("--access_key", help="Access Key Id.")
        parser.add_argument("--secret_key", help="Secret Key.")
        parser.add_argument("--ldapuser", help="Ldap User Id.")
        parser.add_argument("--ldappasswd", help="Ldap Password.")
        parser.add_argument("--session_token", help="Session Token.")
        parser.add_argument("--arn", help="ARN.")
        parser.add_argument("--description", help="Description of the entity.")
        parser.add_argument("--saml_principal_arn", help="SAML Principal ARN.")
        parser.add_argument("--saml_role_arn", help="SAML Role ARN.")
        parser.add_argument("--saml_assertion", help="File conataining SAML assertion.")
        parser.add_argument("--new_user", help="New user name.")
        parser.add_argument("--use-ssl", help="Use HTTPS protocol.", action ='store_true')
        parser.add_argument("--hidden_help",dest = 'hidden_help', action ='store_true', help=argparse.SUPPRESS)
        cli_args = parser.parse_args()

        if cli_args.hidden_help is True and cli_args.action == 'show':
            parser.print_help()
            print(self.iam_usage_hidden())
            sys.exit()

        controller_action = self.load_controller_action()
        """
        Check if the action is valid.
        Note - class name and module name are the same
        """
        try:
            class_name = controller_action[cli_args.action.lower()]['controller']
        except Exception as ex:
            print("Action not found.")
            print(str(ex))
            sys.exit(1)

        if(cli_args.action.lower() in ["createaccount","listaccounts"] ):
            if(cli_args.ldapuser is None):
                print("Provide Ldap User Id.")
                sys.exit(1)

            if(cli_args.ldappasswd is None):
                print("Provide Ldap password.")
                sys.exit(1)
            cli_args.access_key = cli_args.ldapuser
            cli_args.secret_key = cli_args.ldappasswd
        else:
            if(cli_args.access_key is None):
                print("Provide access key.")
                sys.exit(1)

            if(cli_args.secret_key is None):
                print("Provide secret key.")
                sys.exit(1)

         # Get service for the action
        if(not 'service' in controller_action[cli_args.action.lower()].keys()):
            print("Set the service(iam/s3/sts) for the action in the controller_action.yml.")
            sys.exit()
        Config.service = controller_action[cli_args.action.lower()]['service']

        # Load configurations
        self.load_config(cli_args)

        # Create boto3.session object using the access key id and the secret key
        session = self.get_session(cli_args.access_key, cli_args.secret_key, cli_args.session_token)

        # Create boto3.client object.
        client = self.get_client(session)

        # If module is not specified in the controller_action.yaml, then assume
        # class name as the module name.
        if('module' in controller_action[cli_args.action.lower()].keys()):
            module_name = controller_action[cli_args.action.lower()]['module']
        else:
            module_name = class_name.lower()

        try:
            module = self.import_module(module_name)
        except Exception as ex:
            print("Internal error. Module %s not found" % class_name)
            print(str(ex))
            sys.exit(1)

        # Create an object of the controller (user, role etc)
        try:
            controller_obj = self.str_to_class(module, class_name)(client, cli_args)
        except Exception as ex:
            print("Internal error. Class %s not found" % class_name)
            print(str(ex))
            sys.exit(1)

        action = controller_action[cli_args.action.lower()]['action']

        # Call the method of the controller i.e Create, Delete, Update or List
        try:
            getattr(controller_obj, action)()
        except Exception as ex:
            print(str(ex))
            sys.exit(1)
