import subprocess


class EncryptUtil:

    @staticmethod
    def encrypt(plain_text):
        encrypt_cmd = ['java', '-jar', '/opt/seagate/cortx/auth/AuthPassEncryptCLI-1.0-0.jar', '-s', plain_text, '-e', 'aes']
        completed_process = subprocess.check_output(encrypt_cmd)
        return completed_process.decode().rstrip()
