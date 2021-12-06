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
# IMP : for upgrade cmd,
# merge_configs() is imported from the merge.py

from s3confstore.cortx_s3_confstore import S3CortxConfStore
import os.path
import shutil
import sys
import socket
import logging

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
    - All the arrays in yaml are always overwritten
    """

    #If config file is not present then abort merging.
    if not os.path.isfile(configFile):
        logger.error(f'config file {configFile} does not exist')
        raise Exception(f'ERROR: config file {configFile} does not exist')

    logger.info(f'config file {str(configFile)} upgrade started.')

    # old sample file
    conf_old_sample = filetype + oldSampleFile
    cs_conf_old_sample = S3CortxConfStore(config=conf_old_sample, index=conf_old_sample)

    # new sample file
    conf_new_sample = filetype + newSampleFile
    cs_conf_new_sample = S3CortxConfStore(config=conf_new_sample, index=conf_new_sample)
    conf_new_sample_keys = cs_conf_new_sample.get_all_keys()

    # unsafe attribute file
    conf_unsafe_file = filetype + unsafeAttributesFile
    cs_conf_unsafe_file = S3CortxConfStore(config=conf_unsafe_file, index=conf_unsafe_file)
    conf_unsafe_file_keys = cs_conf_unsafe_file.get_all_keys()

    # active config file
    conf_file =  filetype + configFile
    cs_conf_file = S3CortxConfStore(config=conf_file, index=conf_file)
    conf_file_keys = cs_conf_file.get_all_keys()

    #logic to determine which keys to merge.
    keys_to_overwrite = []
    for key in conf_new_sample_keys:
        #If key is marked for unsafe then do not modify/overwrite.
        if key in conf_unsafe_file_keys:
            continue
        #if key not present active config file then add it
        # (this will also add and hence effectively overwrite keys removed in above [] handing
        # and hence will always result in overwrite for these keys from the new sample file).
        if key not in conf_file_keys:
            keys_to_overwrite.append(key)
        #if key is not unsafe and value is not changed by user then overwrite it.
        elif cs_conf_file.get_config(key) == cs_conf_old_sample.get_config(key):
            keys_to_overwrite.append(key)
        #if user has changed the value of the key then skip it.
        else:
            continue

    cs_conf_file.merge_config(source_index=conf_new_sample, keys_to_include=keys_to_overwrite)
    cs_conf_file.save_config()
    logger.info(f'config file {str(configFile)} upgrade completed')

def merge_configs(configFile:str, oldSampleFile:str, newSampleFile:str, unsafeAttributesFile:str, filetype:str):

    """
    - This function will merge all S3 config files during upgrade
    - This function should be used outside this file to call configs upgrade
    """
    # Use existing s3-deployment-logger or setup new console logger
    setup_logger()

    upgrade_config(configFile, oldSampleFile, newSampleFile, unsafeAttributesFile, filetype)

def setup_logger():
    """
    - This function will use as is s3-deployment-logger if it is available
    - else it will log to console
    """
    global logger
    logger = logging.getLogger("s3-deployment-logger-" + "[" + str(socket.gethostname()) + "]")
    if logger.hasHandlers():
        logger.info("Logger has valid handler")
    else:
        logger.setLevel(logging.DEBUG)
        # create console handler with a higher log level
        chandler = logging.StreamHandler(sys.stdout)
        chandler.setLevel(logging.DEBUG)
        s3deployment_log_format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
        formatter = logging.Formatter(s3deployment_log_format)
        # create formatter and add it to the handlers
        chandler.setFormatter(formatter)
        # add the handlers to the logger
        logger.addHandler(chandler)

if __name__ == "__main__":
    print("merge_configs without parameters is not supported, please implement cli wrapper and parser")
    #merge_configs()
