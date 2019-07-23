"""
ObjectRecoveryValidator acts as object validator which performs necessary action for object oid to be deleted.
"""
import logging
import json

from eos_core_kv_api import EOSCoreKVApi
from eos_core_object_api import EOSCoreObjectApi


class ObjectRecoveryValidator:
    """This class is implementation of Validator for object recovery."""

    def __init__(self, config, probable_delete_records,
                 logger=None, objectapi=None, kvapi=None):
        """Initialise Validator"""
        self.config = config
        self.probable_delete_records = probable_delete_records
        if(logger is None):
            self._logger = logging.getLogger("ObjectRecoveryValidator")
        else:
            self._logger = logger
        if(objectapi is None):
            self._objectapi = EOSCoreObjectApi(self.config, self._logger)
        else:
            self._objectapi = objectapi
        if(kvapi is None):
            self._kvapi = EOSCoreKVApi(self.config, self._logger)
        else:
            self._kvapi = kvapi

    def process_results(self):
        """Performs following tasks by processing each of the entries from RabbitMQ:
        1. Checks for probable oid to be deleted with object metadata oid.
        2. Deletes the object oid from object_metadata_index_id if oid matches.
        3. Ignores the object oid from object_metadata_index_id if there is a mismatch."""

        for key, value in self.probable_delete_records.items():
            try:
                probable_delete_object_name = json.loads(value)["obj-name"]
            except ValueError as e:
                self._logger.error(
                    "Failed to parse JSON data for: " + str(value))
                pass

            self._logger.info(
                "Probable object id to be deleted : " +
                probable_delete_object_name)
            result, get_response = self._kvapi.get(
                self.config.get_object_metadata_index_id(), key)
            if(result):
                try:
                    object_metadata = json.loads(get_response.get_value())
                except ValueError as e:
                    self._logger.error(
                        "Failed to parse JSON data for: " + str(get_response.get_value()))
                    pass

                object_metadata_oid = object_metadata["mero_oid_u_hi"]
                if(key == object_metadata_oid):
                    self._logger.info(
                        "Deleting following object Id : " + str(key))
                    self._objectapi.delete(key)
                    self._kvapi.delete(
                        self.config.get_probable_delete_index_id(), key)
                else:
                    self._kvapi.delete(
                        self.config.get_probable_delete_index_id(), key)
                    self._logger.info(
                        "Object name mismatch entry in metadata found for : " +
                        probable_delete_object_name)
            else:
                self._logger.error(
                    "Failed to get object metadata for : " +
                    probable_delete_object_name)
