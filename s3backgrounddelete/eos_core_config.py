import yaml
import sys
import os
import shutil
import logging

class EOSCoreConfig:

    _config = None
    _conf_file = None

    def __init__(self):
        self.logger = logging.getLogger(__name__ + "EOSCoreConfig")
        self._load_and_fetch_config()

    def get_conf_dir(self):
        return os.path.join(os.path.dirname(__file__), 'config')

    def  _load_and_fetch_config(self):
        conf_home_dir = os.path.join('/', 'opt', 'seagate', 's3', 's3backgrounddelete')
        self._conf_file = os.path.join(conf_home_dir,'config.yaml')
        if not os.path.isfile(self._conf_file):
            try:
               os.stat(conf_home_dir)
            except:
               os.mkdir(conf_home_dir)
            shutil.copy(os.path.join(self.get_conf_dir(), 's3_background_delete_config.yaml'), self._conf_file)

        if not os.access(self._conf_file, os.R_OK):
            self.logger.error("Failed to read " + self._conf_file + " it doesn't have read access")
            sys.exit()
        with open(self._conf_file, 'r') as f:
            self._config = yaml.safe_load(f)

    def get_scheduler_logger_name(self):
        if 'logconfig' in self._config and self._config['logconfig']['scheduler_logger_name']:
            return self._config['logconfig']['scheduler_logger_name']
        else :
            raise KeyError("Could not parse scheduler loggername from config file " + self._conf_file)

    def get_processor_logger_name(self):
        if 'logconfig' in self._config and self._config['logconfig']['processor_logger_name']:
            return self._config['logconfig']['processor_logger_name']
        else :
            raise KeyError("Could not parse processor loggername from config file " + self._conf_file)

    def get_scheduler_logger_file(self):
        if 'logconfig' in self._config and self._config['logconfig']['scheduler_log_file']:
            return self._config['logconfig']['scheduler_log_file']
        else :
            raise KeyError("Could not parse scheduler logfile from config file " + self._conf_file)

    def get_processor_logger_file(self):
        if 'logconfig' in self._config and self._config['logconfig']['processor_log_file']:
            return self._config['logconfig']['processor_log_file']
        else :
            raise KeyError("Could not parse processor loggerfile from config file " + self._conf_file)

    def get_file_log_level(self):
        if 'logconfig' in self._config and self._config['logconfig']['file_log_level']:
            return self._config['logconfig']['file_log_level']
        else :
            raise KeyError("Could not parse file loglevel from config file " + self._conf_file)

    def get_console_log_level(self):
        if 'logconfig' in self._config and self._config['logconfig']['console_log_level']:
            return self._config['logconfig']['console_log_level']
        else :
            raise KeyError("Could not parse console loglevel from config file " + self._conf_file)

    def get_log_format(self):
        if 'logconfig' in self._config and self._config['logconfig']['log_format']:
            return self._config['logconfig']['log_format']
        else :
            raise KeyError("Could not parse log format from config file " + self._conf_file)

    def get_eos_core_endpoint(self):
        if 'eos_core' in self._config and self._config['eos_core']['endpoint']:
            return self._config['eos_core']['endpoint']
        else :
            raise KeyError("Could not parse endpoint from config file " + self._conf_file)

    def get_rabbitmq_username(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['username']:
            return self._config['rabbitmq']['username']
        else :
            raise KeyError("Could not parse rabbitmq username from config file " + self._conf_file)

    def get_rabbitmq_password(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['password']:
            return self._config['rabbitmq']['password']
        else :
            raise KeyError("Could not parse rabbitmq password from config file " + self._conf_file)

    def get_rabbitmq_host(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['host']:
            return self._config['rabbitmq']['host']
        else :
            raise KeyError("Could not parse rabbitmq host from config file " + self._conf_file)

    def get_rabbitmq_queue_name(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['queue']:
            return self._config['rabbitmq']['queue']
        else :
            raise KeyError("Could not parse rabbitmq queue from config file " + self._conf_file)

    def get_rabbitmq_exchange(self):
        # The exchange parameter is the name of the exchange. The empty string denotes the default or nameless exchange
        # messages are routed to the queue with the name specified by routing_key, if it exists.
        return self._config['rabbitmq']['exchange']

    def get_rabbitmq_exchange_type(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['Exchange_Type']:
            return self._config['rabbitmq']['Exchange_Type']
        else :
            raise KeyError("Could not parse rabbitmq exchange_type from config file " + self._conf_file)

    def get_rabbitmq_mode(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['mode']:
            return self._config['rabbitmq']['mode']
        else :
            raise KeyError("Could not parse rabbitmq mode from config file " + self._conf_file)

    def get_rabbitmq_durable(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['durable']:
            return self._config['rabbitmq']['durable']
        else :
            raise KeyError("Could not parse rabbitmq durable from config file " + self._conf_file)

    def get_schedule_interval(self):
        if 'rabbitmq' in self._config and self._config['rabbitmq']['schedule_interval_secs']:
            return self._config['rabbitmq']['schedule_interval_secs']
        else :
            raise KeyError("Could not parse rabbitmq schedule interval from config file " + self._conf_file)

    def get_probable_delete_index_id(self):
        if 'indexid' in self._config and self._config['indexid']['probable_delete_index_id']:
            return self._config['indexid']['probable_delete_index_id']
        else :
            raise KeyError("Could not parse probable delete index-id config file " + self._conf_file)

    def get_object_metadata_index_id(self):
        if 'indexid' in self._config and self._config['indexid']['object_metadata_index_id']:
            return self._config['indexid']['object_metadata_index_id']
        else :
            raise KeyError("Could not parse object metadata index-id from config file " + self._conf_file)
