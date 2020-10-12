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

"""
Unit Test for CORTXS3Config class API.
"""
import os
import pytest

from s3backgrounddelete.cortx_s3_config import CORTXS3Config

CONFIG_LOG_DIR = "/var/log/seagate/s3/s3backgrounddelete"


def test_get_config_dir():
    """Test get_config_dir() method """
    config = CORTXS3Config(use_cipher = False)
    config_path = config.get_conf_dir()
    assert os.path.exists(config_path)
    assert os.path.isdir(config_path)


def test_get_scheduler_logger_name_success():
    """Test if scheduler logger name is correct or not"""
    config = CORTXS3Config(use_cipher = False)
    assert config.get_scheduler_logger_name() == "object_recovery_scheduler"


def test_get_scheduler_logger_name_failure():
    """
    Test if scheduler logger name is incorrect then it should throw KeyError
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['logconfig']['scheduler_logger_name']
        assert config.get_scheduler_logger_name()


def test_get_processor_logger_name_success():
    """Test if processor logger name is correct or not"""
    config = CORTXS3Config(use_cipher = False)
    assert config.get_processor_logger_name() == "object_recovery_processor"


def test_get_processor_logger_name_failure():
    """
    Test if processor logger name is incorrect then it should throw KeyError
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['logconfig']['processor_logger_name']
        assert config.get_processor_logger_name()


def test_get_scheduler_logger_file_success():
    """Test if scheduler logger file is returned or not"""
    config = CORTXS3Config(use_cipher = False)
    scheduler_logger_path = CONFIG_LOG_DIR + '/object_recovery_scheduler.log'
    logger_file = config.get_scheduler_logger_file()
    assert logger_file == scheduler_logger_path


def test_get_scheduler_logger_file_failure():
    """
    Test if scheduler logger file is not returned then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['logconfig']['scheduler_log_file']
        assert config.get_scheduler_logger_file()


def test_get_processor_logger_file_success():
    """Test the processor logger file is returned."""
    config = CORTXS3Config(use_cipher = False)
    processor_logger_path = CONFIG_LOG_DIR + '/object_recovery_processor.log'
    logger_file = config.get_processor_logger_file()
    assert logger_file == processor_logger_path


def test_get_processor_logger_file_failure():
    """
    Test if processor logger file is not returned then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['logconfig']['processor_log_file']
        assert config.get_processor_logger_file()


def test_get_log_level_success():
    """Test file log level in logconfig."""
    config = CORTXS3Config(use_cipher = False)
    config._config['logconfig']['file_log_level'] = 5
    file_log_level = config.get_file_log_level()
    assert file_log_level == 5


def test_get_log_level_failure():
    """
    Test if log level of file is not returned then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['logconfig']['file_log_level']
        assert config.get_file_log_level()


def test_get_console_log_level_success():
    """Test console log level in logconfig."""
    config = CORTXS3Config(use_cipher = False)
    config._config['logconfig']['console_log_level'] = 30
    console_log_level = config.get_console_log_level()
    assert console_log_level == 30


def test_get_console_log_level_failure():
    """
    Test if console log level of file is not returned then
    it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['logconfig']['console_log_level']
        assert config.get_console_log_level()


def test_get_log_format_success():
    """Test log format in logconfig."""
    config = CORTXS3Config(use_cipher = False)
    config._config['logconfig']['log_format'] =\
        "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    log_format = config.get_log_format()
    assert log_format == "%(asctime)s - %(name)s - %(levelname)s - %(message)s"


def test_get_log_format_failure():
    """
    Test if log format is not added then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['logconfig']['log_format']
        assert config.get_log_format()


def test_get_cortx_s3_endpoint_success():
    """Test endpoint configuration in cortxs3."""
    config = CORTXS3Config(use_cipher = False)
    config._config['cortx_s3']['endpoint'] = "http://127.0.0.1:7081"
    s3_endpoint = config.get_cortx_s3_endpoint()
    assert s3_endpoint == "http://127.0.0.1:7081"

def test_get_cortx_s3_endpoint_failure():
    """
    Test if endpoint is not configured then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['cortx_s3']['endpoint']
        assert config.get_cortx_s3_endpoint()

def test_get_cortx_s3_service_success():
    """Test service configuration in cortxs3."""
    config = CORTXS3Config(use_cipher = False)
    config._config['cortx_s3']['service'] = "cortxs3"
    s3_service = config.get_cortx_s3_service()
    assert s3_service == "cortxs3"

def test_get_cortx_s3_service_failure():
    """
    Test if service is not configured in cortxs3 configuration
    then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['cortx_s3']['service']
        assert config.get_cortx_s3_service()

def test_get_cortx_s3_region_success():
    """Test default_region configuration in cortxs3."""
    config = CORTXS3Config(use_cipher = False)
    config._config['cortx_s3']['default_region'] = "us-west2"
    s3_region = config.get_cortx_s3_region()
    assert s3_region == "us-west2"

def test_get_cortx_s3_region_failure():
    """
    Test if default_region is not configured in cortxs3 configuration
    then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['cortx_s3']['default_region']
        assert config.get_cortx_s3_region()

def test_get_cortx_s3_access_key_success():
    """Test access_key configuration in cortxs3."""
    config = CORTXS3Config(use_cipher = False)
    config._config['cortx_s3']['background_account_access_key'] = "S_YU-hMoQH2BWtza2tLtVg"
    s3_access_key = config.get_cortx_s3_access_key()
    assert s3_access_key == "S_YU-hMoQH2BWtza2tLtVg"


def test_get_cortx_s3_access_key_failure():
    """
    Test if access_key is not configured in cortxs3 configuration
    then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['cortx_s3']['background_account_access_key']
        assert config.get_cortx_s3_access_key()

def test_get_cortx_s3_access_key_success():
    """Test secret_key configuration in cortxs3."""
    config = CORTXS3Config(use_cipher = False)
    config._config['cortx_s3']['background_account_secret_key'] = "uw13JTMmOFzqz86eaMSbJAFd1CCB7oujkAXX4r+A"
    s3_secret_key = config.get_cortx_s3_secret_key()
    assert s3_secret_key == "uw13JTMmOFzqz86eaMSbJAFd1CCB7oujkAXX4r+A"


def test_get_cortx_s3_secret_key_failure():
    """
    Test if secret_key is not configured in cortxs3 configuration
    then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['cortx_s3']['background_account_secret_key']
        assert config.get_cortx_s3_secret_key()

def test_get_rabbitmq_username_success():
    """Test rabbitmq username."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['username'] = "admin"
    rabbitmq_username = config.get_rabbitmq_username()
    assert rabbitmq_username == "admin"


def test_get_rabbitmq_username_failure():
    """
    Test if rabbitmq username is not specified then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['username']
        assert config.get_rabbitmq_username()


def test_get_rabbitmq_password_success():
    """Test rabbitmq password."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['password'] = "password_admin"
    rabbitmq_password = config.get_rabbitmq_password()
    assert rabbitmq_password == "password_admin"


def test_get_rabbitmq_password_failure():
    """
    Test if rabbitmq password is not specified then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['password']
        assert config.get_rabbitmq_password()


def test_get_rabbitmq_host_success():
    """Test rabbitmq hostname."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['host'] = "107.1.0.1"
    rabbitmq_host = config.get_rabbitmq_host()
    assert rabbitmq_host == "107.1.0.1"


def test_get_rabbitmq_host_failure():
    """
    Test if rabbitmq hostname is not specified then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['host']
        assert config.get_rabbitmq_host()


def test_get_rabbitmq_queue_name_success():
    """Test rabbitmq message queue name."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['queue'] = "s3_delete_obj_job_queue"
    rabbitmq_queue_name = config.get_rabbitmq_queue_name()
    assert rabbitmq_queue_name == "s3_delete_obj_job_queue"


def test_get_rabbitmq_queue_name_failure():
    """
    Test if rabbitmq queuename is not specified then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['queue']
        assert config.get_rabbitmq_queue_name()


def test_get_rabbitmq_exchange_success():
    """Test rabbitmq exchange."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['exchange'] = "test_exchange"
    rabbitmq_exchange = config.get_rabbitmq_exchange()
    assert rabbitmq_exchange == "test_exchange"


def test_get_rabbitmq_exchange_type_success():
    """Test rabbitmq exchange type."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['exchange_type'] = "direct"
    rabbitmq_exchange_type = config.get_rabbitmq_exchange_type()
    assert rabbitmq_exchange_type == "direct"


def test_get_rabbitmq_exchange_type_failure():
    """
    Test if rabbitmq exchange type is not specified
    then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['exchange_type']
        assert config.get_rabbitmq_exchange_type()


def test_get_rabbitmq_mode_success():
    """Test rabbitmq mode."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['mode'] = 1
    rabbitmq_mode = config.get_rabbitmq_mode()
    assert rabbitmq_mode == 1


def test_get_rabbitmq_mode_failure():
    """
    Test if rabbitmq mode is not specified then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['mode']
        assert config.get_rabbitmq_mode()


def test_get_rabbitmq_durable_success():
    """Test rabbitmq durable."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['durable'] = "True"
    rabbitmq_durable = config.get_rabbitmq_durable()
    assert rabbitmq_durable == "True"


def test_get_rabbitmq_durable_failure():
    """
    Test if rabbitmq durable is not specified then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['durable']
        assert config.get_rabbitmq_durable()


def test_get_schedule_interval_success():
    """Test if scheduler time interval is returned."""
    config = CORTXS3Config(use_cipher = False)
    config._config['rabbitmq']['schedule_interval_secs'] = 900
    schedule_interval = config.get_schedule_interval()
    assert schedule_interval == 900


def test_get_schedule_interval_failure():
    """
    Test if scheduler time interval is not specified
    then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['rabbitmq']['schedule_interval_secs']
        assert config.get_schedule_interval()


def test_get_probable_delete_index_id_success():
    """Test if probable delete index id is returned."""
    config = CORTXS3Config(use_cipher = False)
    config._config['indexid']['probable_delete_index_id'] = "test_index"
    index_id = config.get_probable_delete_index_id()
    assert index_id == "test_index"


def test_get_probable_delete_index_id_failure():
    """
    Test if probable delete index id is not specified
    then it should throw KeyError.
    """
    with pytest.raises(KeyError):
        config = CORTXS3Config(use_cipher = False)
        del config._config['indexid']['probable_delete_index_id']
        assert config.get_probable_delete_index_id()
