"""
ObjectRecoveryValidator acts as object validator which performs necessary action for object oid to be deleted.
"""
import logging
import json

from s3backgrounddelete.eos_core_kv_api import EOSCoreKVApi
from s3backgrounddelete.eos_core_object_api import EOSCoreObjectApi
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi


class ObjectRecoveryValidator:
    """This class is implementation of Validator for object recovery."""

    def __init__(self, config, probable_delete_records,
                 logger=None, objectapi=None, kvapi=None, indexapi=None):
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
        if(indexapi is None):
            self._indexapi = EOSCoreIndexApi(self.config, logger=self._logger)
        else:
            self._indexapi = indexapi

    def check_instance_is_nonactive(self, instance_id, marker=None):

        """Checks for existence of instance_id inside global instance index"""

        result, instance_response = self._indexapi.list(
                    self.config.get_global_instance_index_id(), self.config.get_max_keys(), marker)
        if(result):
            # List global instance index is success.
            self._logger.info("Index listing result :" +
                             str(instance_response.get_index_content()))
            global_instance_json = instance_response.get_index_content()
            global_instance_list = global_instance_json["Keys"]
            is_truncated = global_instance_json["IsTruncated"]

            if(global_instance_list is not None):
                for record in global_instance_list:
                    if(record["Value"] == instance_id):
                        # instance_id found. Skip entry and retry for delete oid again.
                        self._logger.info("S3 Instance is still active. Skipping delete operation")
                        return False

                if(is_truncated == "true"):
                    self.check_instance_is_nonactive(instance_id, global_instance_json["NextMarker"])

            # List global instance index results is empty hence instance_id not found.
            return True

        else:
            # List global instance index is failed.
            self._logger.error("Failed to list global instance index")
            return False

    def process_results(self):
        """Performs following tasks by processing each of the entries from RabbitMQ:
        1. Checks that the S3 instance id which performes write operations is not active.
        2. Checks for probable oid to be deleted with object metadata oid.
        3. Deletes the object oid from object_metadata_index_id if oid matches.
        4. Ignores the object oid from object_metadata_index_id if there is a mismatch."""

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
        instance_id = probable_delete_json["global_instance_id"]

        if(self.check_instance_is_nonactive(instance_id)):
        # S3 instance performing write operation is not active. Safe to proceed with further validation

            delete_probable_oid = False

            self._logger.info(
                "Fetching object metadata corresponding to index id: " +
                index_id +
                " and key: " +
                object_metadata_path)

            ret, response_data = self._kvapi.get(index_id, object_metadata_path)
            # Object Metadata exists. Verify the probable delete oid with oid present in object metadata
            if(ret):
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
            elif(ret is False):
                if(response_data.get_error_status() == 404):
                    self._logger.info("Response for object metadata corresponding to object id" + str(
                        response_data.get_error_status()) + " " + str(response_data.get_error_message()))

                    self._logger.info(
                        "Deleting following object Id : " +
                        str(probable_delete_oid))

                    delete_probable_oid, delete_response = self._objectapi.delete(
                        probable_delete_oid, object_layout_id)

                # Handel if fetching object metadata returns exception other than 404 Not Found
                else:
                    self._logger.error(
                    "Failed to fetch Object metadata for following object Id " +
                    str(probable_delete_oid) +
                    "Ignoring oid delete from probable delete index id")
                    self._logger.error("Fetch object metadata failed due to : " + str(response_data.get_error_message()))
                    return

            if(delete_probable_oid):
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
        else:
            # S3 instance is active which might be performing write operation. Skip entry and retry again
            self._logger.info("Retrying delete operation as S3 instance id is still active")


