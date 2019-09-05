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
            self._objectapi = EOSCoreObjectApi(self.config, logger=self._logger)
        else:
            self._objectapi = objectapi
        if(kvapi is None):
            self._kvapi = EOSCoreKVApi(self.config, logger=self._logger)
        else:
            self._kvapi = kvapi

    def process_results(self):
        """Performs following tasks by processing each of the entries from RabbitMQ:
        1. Checks for probable oid to be deleted with object metadata oid.
        2. Deletes the object oid from object_metadata_index_id if oid matches.
        3. Ignores the object oid from object_metadata_index_id if there is a mismatch."""

        probable_delete_oid = self.probable_delete_records["Key"]
        probable_delete_value = self.probable_delete_records["Value"]

        self._logger.info(
            "Probable object id to be deleted : " +
            probable_delete_oid)
        try:
            probable_delete_json = json.loads(probable_delete_value)
        except ValueError as error:
            self._logger.error(
                "Failed to parse JSON data for: " + probable_delete_value + " due to: " + error)
            return
        index_id = probable_delete_json["index_id"]
        object_layout_id = probable_delete_json["object_layout_id"]
        object_metadata_path = probable_delete_json["object_metadata_path"]
        delete_probable_oid = False

        self._logger.info(
            "Fetching object metadata corresponding to index id: " +
            index_id +
            " and key: " +
            object_metadata_path)

        ret, response_data = self._kvapi.get(index_id, object_metadata_path)
        # Object Metadata exists. Verify the probable delete oid with oid present in object metadata
        if (ret):
            try:
                object_metadata = json.loads(response_data.get_value())
                self._logger.info(
                    "Object metadata corresponding to object id" +
                    str(object_metadata))
                object_metadata_oid = object_metadata["mero_oid"]
                # Object oid from object metadata equals probable delete oid
                if(object_metadata_oid == probable_delete_oid):
                    self._logger.info(
                        "Ignoring deletion for object id" +
                        str(probable_delete_oid) +
                        " as it matches object metadata oid ")
                    delete_probable_oid = True
                    pass
                else:
                    self._logger.info(
                        "Deleting following object id" +
                        str(probable_delete_oid) +
                        " as it mismatches object metadata oid ")
                    delete_probable_oid, delete_response = self._objectapi.delete(
                        probable_delete_oid, object_layout_id)
            except ValueError as e:
                self._logger.error(
                    "Failed to parse JSON data for: " + str(response_data.get_value()))
                return
        # Object Metadata doesn't exists. Proceed with deletion of probable delete object oid
        elif (ret is False):
              if(response_data.get_error_status() == 404):
                 self._logger.info("Response for object metadata corresponding to object id" + str(
                      response_data.get_error_status()) + " " + str(response_data.get_error_message()))

                 self._logger.info(
                    "Deleting following object Id : " +
                     str(probable_delete_oid))

                 delete_probable_oid, delete_response = self._objectapi.delete(
                     probable_delete_oid, object_layout_id)

              #Handel if fetching object metadata returns exception other than 404 Not Found
              else:
                  self._logger.error(
                  "Failed to fetch Object metadata for following object Id " +
                  str(probable_delete_oid) +
                  "Ignoring oid delete from probable delete index id")
                  self._logger.error("Fetch object metadata failed due to : " + str(response_data.get_error_message()))
                  return

        if (delete_probable_oid):
            # Delete oid successful delete the oid entry from probable delete index id
            self._kvapi.delete(
                self.config.get_probable_delete_index_id(),
                probable_delete_oid)
        else:
            # Delete oid failed skip deletion of oid entry from probable delete index id
            self._logger.error(
                "Failed to delete following object Id " +
                str(probable_delete_oid) +
                "Ignoring oid delete from probable delete index id")
            self._logger.error("Object deletion failed due to : " + str(delete_response.get_error_message()))

