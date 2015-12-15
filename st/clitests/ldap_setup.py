import os
import yaml
from subprocess import call

class LdapSetup:
    def __init__(self):
        self.tools_dir = os.path.join(os.path.dirname(__file__), 'tools')

        ldap_config_file = os.path.join(self.tools_dir, 'ldap_config.yaml')
        with open(ldap_config_file, 'r') as f:
            self.ldap_config = yaml.safe_load(f)

    def ldap_init(self):
        ldap_init_file = os.path.join(self.tools_dir, 'ldap_init.ldif')
        cmd = "ldapadd -h %s -p %s -w %s -x -D %s -f %s" % (self.ldap_config['host'],
                self.ldap_config['port'], self.ldap_config['password'],
                self.ldap_config['login_dn'], ldap_init_file)
        obj = call(cmd, shell=True)

    def create_ldap_test_account(self):
        ldap_init_file = os.path.join(self.tools_dir, 's3_test_account.ldif')
        cmd = "ldapadd -h %s -p %s -w %s -x -D %s -f %s" % (self.ldap_config['host'],
                self.ldap_config['port'], self.ldap_config['password'],
                self.ldap_config['login_dn'], ldap_init_file)
        call(cmd)

    def ldap_delete_all(self):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r \"dc=seagate,dc=com\"" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn']))
        obj = call(cmd, shell=True)

    def delete_ldap_test_account(self):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r \
                \"o=s3test,ou=accounts,dc=s3,dc=seagate,dc=com\" \
                \"ak=AKIAJTYX36YCKQSAJT7Q,ou=accesskeys,dc=s3,dc=seagate,dc=com\"" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn']))
        obj = call(cmd)
