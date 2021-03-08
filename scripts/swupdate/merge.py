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
import os.path
import shutil

g_upgrade_items = {
  's3' : {
        'configFile' : "/opt/seagate/cortx/s3/conf/s3config.yaml",
        'oldSampleFile' : "/tmp/s3config.yaml.sample.old",
        'newSampleFile' : "/opt/seagate/cortx/s3/conf/s3config.yaml.sample",
        'unsafeAttributesFile' : "/opt/seagate/cortx/s3/conf/s3config_unsafe_attributes.yaml",
        'fileType' : 'yaml://'
    },
    'auth' : {
        'configFile' : "/opt/seagate/cortx/auth/resources/authserver.properties",
        'oldSampleFile' : "/tmp/authserver.properties.sample.old",
        'newSampleFile' : "/opt/seagate/cortx/auth/resources/authserver.properties.sample",
        'unsafeAttributesFile' : "/opt/seagate/cortx/auth/resources/authserver_unsafe_attributes.properties",
        'fileType' : 'properties://'
    },
    'keystore' : {
        'configFile' : "/opt/seagate/cortx/auth/resources/keystore.properties",
        'oldSampleFile' : "/tmp/keystore.properties.sample.old",
        'newSampleFile' : "/opt/seagate/cortx/auth/resources/keystore.properties.sample",
        'unsafeAttributesFile' : "/opt/seagate/cortx/auth/resources/keystore_unsafe_attributes.properties",
        'fileType' : 'properties://'
    },
    'bgdelete' : {
        'configFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml",
        'oldSampleFile' : "/tmp/config.yaml.sample.old",
        'newSampleFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample",
        'unsafeAttributesFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/s3backgrounddelete_unsafe_attributes.yaml",
        'fileType' : 'yaml://'
    }
}

def upgrade_config(configFile:str, oldSampleFile:str, newSampleFile:str, unsafeAttributesFile:str, filetype:str):
    """
    Core logic for updating config files during upgrade using conf store.
    Following is algorithm from merge:
    Iterate over all parameters sample.new file
    for every parameter, check
    - if it is marked as 'unsafe' in attributes file, skip
    - if it marked as 'safe' in the attributes file  
        - diff the value in config and sample.old - if it is changed, skip
        - if it is not changed,  we will overwrite the value in cfg file from sample.new
        - if it does not exist in cfg file add the value from sample.new file to cfg file
    """

    #If config file is not present then simply copy sample file.
    if not os.path.isfile(configFile):
        shutil.copy(newSampleFile, configFile)
        return

    conf_file =  filetype + configFile
    conf_old_sample = filetype + oldSampleFile
    conf_new_sample = filetype + newSampleFile
    conf_unsafe_file = filetype + unsafeAttributesFile

    cs_conf_file = S3CortxConfStore(config=conf_file, index=conf_file)
    cs_conf_old_sample = S3CortxConfStore(config=conf_old_sample, index=conf_old_sample)
    cs_conf_new_sample = S3CortxConfStore(config=conf_new_sample, index=conf_new_sample)
    cs_conf_unsafe_file = S3CortxConfStore(config=conf_unsafe_file, index=conf_unsafe_file)

    conf_file_keys = cs_conf_file.get_all_keys()
    conf_unsafe_file_keys = cs_conf_unsafe_file.get_all_keys()
    conf_new_sample_keys = cs_conf_new_sample.get_all_keys()

    # Handle the special scenario where we have array of dictionaries in the config file 
    # 1)search for keys with [] in config
    # 2)delete these keys/values in config
    for key in conf_file_keys:
        if ((key.find('[') != -1) and (key.find(']') != -1)):
            cs_conf_file.delete_key(key, True)

    #logic to determine which keys to merge.
    keys_to_overwrite = []
    for key in conf_new_sample_keys:
        #check if the key is safe for modification.
        if key in conf_unsafe_file_keys:
            continue
        #if key not present in config then add it to config.
        if key not in conf_file_keys:
            keys_to_overwrite.append(key)
        #if config value == old sample then change to new value.
        elif cs_conf_file.get_config(key) == cs_conf_old_sample.get_config(key):
            keys_to_overwrite.append(key)
        #if user has changed the value of this key then skip it.
        else:
            continue

    cs_conf_file.merge_config(source_index=conf_new_sample, keys_to_include=keys_to_overwrite)
    cs_conf_file.save_config()

if __name__ == "__main__":
    for upgrade_item in g_upgrade_items:
        upgrade_config(g_upgrade_items[upgrade_item]['configFile'],
            g_upgrade_items[upgrade_item]['oldSampleFile'],
            g_upgrade_items[upgrade_item]['newSampleFile'],
            g_upgrade_items[upgrade_item]['unsafeAttributesFile'],
            g_upgrade_items[upgrade_item]['fileType'])
