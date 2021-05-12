#!/usr/bin/python3.6
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

from s3confstore.cortx_s3_confstore import S3CortxConfStore
import sys

g_upgrade_items = {
  's3' : {
        'configFile' : "/opt/seagate/cortx/s3/conf/s3config.yaml",
        'SampleFile' : "/opt/seagate/cortx/s3/conf/s3config.yaml.sample",
        'fileType' : 'yaml://'
    },
    'auth' : {
        'configFile' : "/opt/seagate/cortx/auth/resources/authserver.properties",
        'SampleFile' : "/opt/seagate/cortx/auth/resources/authserver.properties.sample",
        'fileType' : 'properties://'
    },
    'keystore' : {
        'configFile' : "/opt/seagate/cortx/auth/resources/keystore.properties",
        'SampleFile' : "/opt/seagate/cortx/auth/resources/keystore.properties.sample",
        'fileType' : 'properties://'
    },
    'bgdelete' : {
        'configFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml",
        'SampleFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample",
        'fileType' : 'yaml://'
    },
    'cluster' : {
        'configFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml",
        'SampleFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml.sample",
        'fileType' : 'yaml://'
    }
}

def validate_config(configFile:str, SampleFile:str, filetype:str):
    """
    Validate the sample file and config file keys.
    Both files should have same keys.
    if keys mismatch then there is some issue in the merging config file logic 
    """

    sys.stdout.write(f'INFO: validating config file {str(configFile)}.\n')

    # new sample file
    conf_sample = filetype + SampleFile
    cs_conf_sample = S3CortxConfStore(config=conf_sample, index=conf_sample)
    conf_sample_keys = cs_conf_sample.get_all_keys()

    # active config file
    conf_file =  filetype + configFile
    cs_conf_file = S3CortxConfStore(config=conf_file, index=conf_file)
    conf_file_keys = cs_conf_file.get_all_keys()

    # compare the keys of sample file and config file 
    if conf_sample_keys == conf_file_keys:
        sys.stdout.write(f'INFO: config file {str(configFile)} validate successfully.\n')
    else:
        sys.stderr.write(f'ERROR: config file {str(conf_file)} and sample file {str(conf_sample)} keys does not matched.\n')
        sys.stderr.write(f'ERROR: sample file keys: {str(conf_sample_keys)}\n')
        sys.stderr.write(f'ERROR: config file keys: {str(conf_file_keys)}\n')
        raise Exception(f'ERROR: Failed to validate config file {str(configFile)}.\n')

if __name__ == "__main__":
    try:
        for upgrade_item in g_upgrade_items:
            validate_config(g_upgrade_items[upgrade_item]['configFile'],
                g_upgrade_items[upgrade_item]['SampleFile'],
                g_upgrade_items[upgrade_item]['fileType'])
    except:
        raise