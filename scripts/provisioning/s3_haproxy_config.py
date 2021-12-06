#!/usr/bin/env python3
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#

import os
import shutil
import sys
import socket
import logging
import urllib.parse
from ast import literal_eval
from setupcmd import S3PROVError
from s3confstore.cortx_s3_confstore import S3CortxConfStore

class S3HaproxyConfig:
  """HAProxy configration for S3."""
  local_confstore = None
  provisioner_confstore = None
  machine_id = None

  def __init__(self, confstore: str):
    """Constructor."""

    s3deployment_logger_name = "s3-deployment-logger-" + "[" + str(socket.gethostname()) + "]"
    self.logger = logging.getLogger(s3deployment_logger_name)
    if self.logger.hasHandlers():
      self.logger.info("Logger has valid handler")
    else:
      self.logger.setLevel(logging.DEBUG)
      # create console handler with a higher log level
      chandler = logging.StreamHandler(sys.stdout)
      chandler.setLevel(logging.DEBUG)
      s3deployment_log_format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
      formatter = logging.Formatter(s3deployment_log_format)
      # create formatter and add it to the handlers
      chandler.setFormatter(formatter)
      # add the handlers to the logger
      self.logger.addHandler(chandler)

    if not confstore.strip():
      self.logger.error(f'config url:[{confstore}] must be a valid url path')
      raise Exception('empty config URL path')

    self.provisioner_confstore = S3CortxConfStore(confstore, 'haproxy_config_index')

    # Get machine-id of current node from constore
    self.machine_id = self.provisioner_confstore.get_machine_id()
    self.logger.info(f'Machine id : {self.machine_id}')

  @staticmethod
  def parse_endpoint(endpoint_str):

    """Parse endpoint string and return dictionary with components:
         * scheme,
         * fqdn,
         * and optionally port, if present in the string.

       Examples:

       'https://s3.seagate.com:443' -> {'scheme': 'https', 'fqdn': 's3.seagate.com', 'port': '443'}
       'https://s3.seagate.com'     -> {'scheme': 'https', 'fqdn': 's3.seagate.com'}
       'http://s3.seagate.com:80'   -> {'scheme': 'http', 'fqdn': 's3.seagate.com', 'port': '80'}
       'http://127.0.0.1:80'        -> {'scheme': 'http', 'fqdn': '127.0.0.1', 'port': '80'}
    """
    try:
      result1 = urllib.parse.urlparse(endpoint_str)
      result2 = result1.netloc.split(':')
      result = { 'scheme': result1.scheme, 'fqdn': result2[0] }
      if len(result2) > 1:
        result['port'] = result2[1]
    except Exception as e:
      raise S3PROVError(f'Failed to parse endpoing {endpoint_str}.  Exception: {e}')
    return result

  def get_endpoint_for_scheme(self, confstore_key_value, scheme):
    """Scan list of endpoints, and return parsed endpoint for a given scheme."""
    if not isinstance(confstore_key_value, str):
      lst=confstore_key_value
    else:
      lst=[confstore_key_value]
    endpoint_str = ""
    for endpoint_str in lst:
      print (endpoint_str)
      endpoint = self.parse_endpoint(endpoint_str)
      if endpoint['scheme'] == scheme:
        return endpoint
    return None

  def get_publicip(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return self.get_config_with_defaults('CONFIG>CONFSTORE_PUBLIC_FQDN_KEY')

  def get_privateip(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return self.get_config_with_defaults('CONFIG>CONFSTORE_PRIVATE_FQDN_KEY')

  def get_s3instances(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return int(self.get_config_with_defaults('CONFIG>CONFSTORE_S3INSTANCES_KEY'))

  def get_endpoint_port(self, confstore_key, endpoint_type):
    confstore_key_value = self.get_config_with_defaults(confstore_key)
    if isinstance(confstore_key_value, str):
      confstore_key_value = literal_eval(confstore_key_value)
    endpoint = self.get_endpoint_for_scheme(confstore_key_value, endpoint_type)
    if endpoint is None:
      raise S3PROVError(f"{confstore_key} does not have any specified endpoint type : {endpoint_type}")
    if 'port' not in endpoint:
      raise S3PROVError(f"{confstore_key} does not specify endpoint port {endpoint} for endpoint type {endpoint_type}")
    return endpoint['port']

  def get_s3server_backend_port(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return int(self.get_config_with_defaults('CONFIG>CONFSTORE_S3SERVER_PORT'))

  def get_s3auth_backend_port(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return int(self.get_config_with_defaults('CONFIG>CONFSTORE_S3_AUTHSERVER_PORT'))

  def get_sslcertpath(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return self.get_config_with_defaults('CONFIG>CONFSTORE_S3_SECURITY_CERTIFICATE')

  def get_baseconfig_path(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return self.get_config_with_defaults('CONFIG>CONFSTORE_BASE_CONFIG_PATH')

  def get_confkey(self, key: str):
    assert self.local_confstore != None
    return self.local_confstore.get_config(key)

  def get_config_with_defaults(self, key: str):
    confkey = self.local_confstore.get_config(key)
    if "machine-id" in confkey:
      confkey = confkey.replace("machine-id", self.machine_id) 
    confkey_value = self.provisioner_confstore.get_config(confkey)

    if confkey_value is None:
      confkey_value = self.local_confstore.get_config("DEFAULT_" + key)

    return confkey_value


  def process(self):

    self.local_confstore = S3CortxConfStore(
    "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml",
    'localstore')

    setup_type = str(self.get_config_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE'))

    self.logger.info(f'Setup type is {setup_type}')

    if ("K8" == setup_type) :
      self.configure_haproxy_k8()
    else : 
      self.configure_haproxy_legacy()

  def configure_haproxy_k8(self):
    self.logger.info("K8s HAPROXY configuration ...")

    # Fetching base config path
    baseconfig_path = self.get_baseconfig_path()

    # Fetching ssl certificate path
    sslcertpath = self.get_sslcertpath()

    # Fetching S3 server instances
    numS3Instances = self.get_s3instances()
    if numS3Instances <= 0:
        numS3Instances = 1

    # Fetching frontend service ports
    s3fendhttpport = self.get_endpoint_port("CONFIG>CONFSTORE_S3_DATA_ENDPOINTS", "http")
    s3fendhttpsport = self.get_endpoint_port("CONFIG>CONFSTORE_S3_DATA_ENDPOINTS", "https")
    s3authfendhttpport = self.get_endpoint_port("CONFIG>CONFSTORE_S3_IAM_ENDPOINTS", "http")
    s3authfendhttpsport = self.get_endpoint_port("CONFIG>CONFSTORE_S3_IAM_ENDPOINTS", "https")
    s3bgdeleteport = self.get_endpoint_port("CONFIG>CONFSTORE_S3_INTERNAL_ENDPOINTS", "http")

    # Fetching backend service ports
    s3backendport = self.get_s3server_backend_port()
    s3authbendport = self.get_s3auth_backend_port()

    config_file = os.path.join(baseconfig_path, 's3/haproxy.cfg')
    errors_file = self.get_confkey("S3_HAPROXY_ERROR_CONFIG_FILE")
    global_text = '''global
    log         127.0.0.1 local2
    log stdout format raw local0
    h1-case-adjust authorization Authorization
    h1-case-adjust accept-encoding Accept-Encoding
    h1-case-adjust action Action
    h1-case-adjust credential Credential
    h1-case-adjust signedHeaders SignedHeaders
    h1-case-adjust signature Signature
    h1-case-adjust clientAbsoluteUri ClientAbsoluteUri
    h1-case-adjust clientQueryParams ClientQueryParams
    h1-case-adjust connection Connection
    h1-case-adjust host Host
    h1-case-adjust method Method
    h1-case-adjust request_id Request_id
    h1-case-adjust requestorAccountId RequestorAccountId
    h1-case-adjust requestorAccountName RequestorAccountName
    h1-case-adjust requestorCanonicalId RequestorCanonicalId
    h1-case-adjust requestorEmail RequestorEmail
    h1-case-adjust requestorUserId RequestorUserId
    h1-case-adjust requestorUserName RequestorUserName
    h1-case-adjust s3Action S3Action
    h1-case-adjust user-Agent User-Agent
    h1-case-adjust version Version
    h1-case-adjust x-Amz-Content-SHA256 X-Amz-Content-SHA256
    h1-case-adjust x-Amz-Date X-Amz-Date
    h1-case-adjust x-Forwarded-For X-Forwarded-For

    chroot      /var/lib/haproxy
    pidfile     /var/run/haproxy.pid
    maxconn     13
    user        haproxy
    group       haproxy
    nbproc      2
    daemon

    # turn on stats unix socket and dynamic configuration reload
    stats socket /var/lib/haproxy/stats expose-fd listeners

    # utilize system-wide crypto-policies
    # ssl-default-bind-ciphers PROFILE=SYSTEM
    # ssl-default-server-ciphers PROFILE=SYSTEM

    #SSL options            # utilize system-wide crypto-policies
    tune.ssl.default-dh-param 2048
'''
    default_text = '''
#---------------------------------------------------------------------
# common defaults that all the 'listen' and 'backend' sections will
# use if not designated in their block
#---------------------------------------------------------------------
defaults
    mode                    http
    log                     global
    option                  httplog
    option                  dontlognull
    option h1-case-adjust-bogus-server
    option h1-case-adjust-bogus-client
    option http-server-close
    option forwardfor       except 127.0.0.0/8
    option                  redispatch
    errorfile               503 %s
    retries                 3
    timeout http-request    10s
    timeout queue           10s
    timeout connect         5s
    timeout client          360s
    timeout server          360s
    timeout http-keep-alive 10s
    timeout check           10s
    maxconn                 3000
'''
    frontend_s3main_text = '''
#----------------------------------------------------------------------
# FrontEnd S3 Configuration
#----------------------------------------------------------------------
frontend s3-main
    # s3 server port
'''
    backend_s3main_text = '''
#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorithm
#----------------------------------------------------------------------
backend s3-main
    balance static-rr                                     #Balance algorithm
    http-response set-header Server SeagateS3
    # Check the S3 server application is up and healthy - 200 status code
    option httpchk HEAD / HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter

    # For ssl communication between haproxy and s3server
    # Replace below line

'''
    backend_s3bgdelete_text = '''
#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorithm for s3 bgdelete server
#----------------------------------------------------------------------
backend s3-bgdelete
    balance static-rr                                     #Balance algorithm
'''
    backend_s3auth_text = '''
#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorith for s3 auth server
#----------------------------------------------------------------------
backend s3-auth
    balance static-rr                                     #Balance algorithm

    # Check the S3 Auth server application is up and healthy - 200 status code
    option httpchk HEAD /auth/health HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter
'''
    config_handle = open(config_file, "w+")
    config_handle.write(global_text)
    config_handle.write(default_text % errors_file)
    config_handle.write(frontend_s3main_text)
    config_handle.write(
         "   bind 0.0.0.0:%s\n"
         "   bind 0.0.0.0:%s ssl crt %s\n"
         "\n"
         "   option forwardfor\n"
         "   default_backend s3-main\n"
         "\n"
         "   # s3 bgdelete server port\n"
         "   bind 0.0.0.0:%s\n"
         "   acl s3bgdeleteacl dst_port %s\n"
         "   use_backend s3-bgdelete if s3bgdeleteacl\n"
         "\n"
         "   # s3 auth server port\n"
         "   bind 0.0.0.0:%s\n"
         "   bind 0.0.0.0:%s ssl crt %s\n"
         "\n"
         "   acl s3authbackendacl dst_port %s\n"
         "   acl s3authbackendacl dst_port %s\n"
         "   use_backend s3-auth if s3authbackendacl\n"
         % (s3fendhttpport, s3fendhttpsport, sslcertpath, s3bgdeleteport, s3bgdeleteport, s3authfendhttpport, s3authfendhttpsport, sslcertpath, s3authfendhttpsport, s3authfendhttpport))
    config_handle.write(backend_s3main_text)
    for i in range(0, numS3Instances):
        config_handle.write(
        "    server s3-instance-%s 0.0.0.0:%s check maxconn 110        # s3 instance %s\n"
        % (i+1, s3backendport+i, i+1))
    config_handle.write(backend_s3bgdelete_text)
    config_handle.write(
        "    server s3-bgdelete-instance-1 0.0.0.0:%s           # s3 bgdelete instance 1\n"
        % (int(s3bgdeleteport)-1))
    config_handle.write(backend_s3auth_text)
    config_handle.write(
        "    server s3authserver-instance1 0.0.0.0:%s #check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance 1\n"
        "\n"
        "#-------S3 Haproxy configuration end-----------------------------------\n"
        % (s3authbendport))
    config_handle.close()

    #Check for destination dirs and create if needed
    pem_dir = os.path.dirname(sslcertpath)
    if not os.path.exists(pem_dir):
        os.makedirs(pem_dir)

  def configure_haproxy_legacy(self):
    """Main Processing function."""
    self.logger.info("Legacy HAPROXY configuration ...")

    #Get necessary info from confstore
    localhost = '127.0.0.1'
    pvt_ip = self.get_privateip()
    pub_ip = self.get_publicip()
    numS3Instances = self.get_s3instances()

    if numS3Instances <= 0:
        numS3Instances = 1

    #Initialize header and footer text
    header_text = "#-------S3 Haproxy configuration start---------------------------------"
    footer_text = "#-------S3 Haproxy configuration end-----------------------------------"

    #Remove all existing content from header to footer
    is_found = False
    header_found = False
    original_file = "/etc/haproxy/haproxy.cfg"
    dummy_file = original_file + '.bak'
    # Open original file in read only mode and dummy file in write mode
    with open(original_file, 'r') as read_obj, open(dummy_file, 'w') as write_obj:
        # Line by line copy data from original file to dummy file
        for line in read_obj:
            line_to_match = line
            if line[-1] == '\n':
                line_to_match = line[:-1]
            # if current line matches with the given line then skip that line
            if line_to_match != header_text:
                if header_found == False:
                    write_obj.write(line)
                elif line_to_match == footer_text:
                     header_found = False
            else:
                is_found = True
                header_found = True
                break
    read_obj.close()
    write_obj.close()
    # If any line is skipped then rename dummy file as original file
    if is_found:
        os.remove(original_file)
        os.rename(dummy_file, original_file)
    else:
        os.remove(dummy_file)

    #Set the string literals to be added to config file
    frontend_s3main_text = '''
#----------------------------------------------------------------------
# FrontEnd S3 Configuration
#----------------------------------------------------------------------
frontend s3-main
    # s3 server port
'''
    backend_s3main_text = '''

    option forwardfor
    default_backend s3-main

    # s3 auth server port
    bind 0.0.0.0:9080
    bind 0.0.0.0:9443 ssl crt /etc/ssl/stx/stx.pem

    acl s3authbackendacl dst_port 9443
    acl s3authbackendacl dst_port 9080
    use_backend s3-auth if s3authbackendacl

#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorithm
#----------------------------------------------------------------------
backend s3-main
    balance static-rr                                     #Balance algorithm
    http-response set-header Server SeagateS3
    # Check the S3 server application is up and healthy - 200 status code
    option httpchk HEAD / HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter

    # For ssl communication between haproxy and s3server
    # Replace below line
'''

    backend_s3auth_text = '''
#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorith for s3 auth server
#----------------------------------------------------------------------
backend s3-auth
    balance static-rr                                     #Balance algorithm

    # Check the S3 Auth server application is up and healthy - 200 status code
    option httpchk HEAD /auth/health HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter

'''

    new_line_text = '''
'''

    #Initialize port numbers
    s3backendport = self.get_s3server_backend_port()
    self.logger.info(f'S3 server backend port: {s3backendport}')
    s3aubackendport = self.get_s3auth_backend_port()
    self.logger.info(f'S3 auth server backend port: {s3aubackendport}')

    #Add complete information to haproxy.cfg file
    cfg_file = '/etc/haproxy/haproxy.cfg'
    target = open(cfg_file, "a+")

    target.write(header_text)
    target.write(frontend_s3main_text)
    target.write(
        "    bind %s:80 ##### localhost 80 required for Auth - S3 connection\n"
        "    bind %s:443 ssl crt /etc/ssl/stx/stx.pem ### localhost required for CSM/UDX\n"
        "    bind %s:80\n"
        "    bind %s:443 ssl crt /etc/ssl/stx/stx.pem\n"
        "    bind %s:80\n"
        "    bind %s:443 ssl crt /etc/ssl/stx/stx.pem\n"
        % (localhost, localhost, pvt_ip, pvt_ip, pub_ip, pub_ip))
    target.write(backend_s3main_text)
    for i in range(0, numS3Instances):
        target.write(
        "    server s3-instance-%s %s:%s check maxconn 110        # s3 instance %s\n"
        % (i+1, pvt_ip, s3backendport+i, i+1))
    target.write(backend_s3auth_text)
    for i in range(0, 1):
        target.write(
        "    server s3authserver-instance%s %s:%s #check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance %s\n"
        % (i+1, pvt_ip, s3aubackendport+i, i+1))
    target.write(new_line_text)
    target.write(footer_text)

    target.close()

    #Check for destination dirs and create if needed
    if not os.path.exists('/etc/haproxy/errors/'):
        os.makedirs('/etc/haproxy/errors/')

    #Run config commands
    shutil.copyfile("/opt/seagate/cortx/s3/install/haproxy/503.http", "/etc/haproxy/errors/503.http")
    shutil.copyfile("/opt/seagate/cortx/s3/install/haproxy/logrotate/haproxy", "/etc/logrotate.d/haproxy")
    shutil.copyfile("/opt/seagate/cortx/s3/install/haproxy/rsyslog.d/haproxy.conf", "/etc/rsyslog.d/haproxy.conf")
    shutil.copyfile("/opt/seagate/cortx/s3/install/haproxy/logrotate/logrotate", "/etc/cron.hourly/logrotate")
    os.system("systemctl restart rsyslog")
