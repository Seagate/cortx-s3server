import json

class EOSCoreIndexResponse:
    _status = 200
    _err_message = ""
    _key = ""

    def __init__(self, status, msg, index_content):
        self._status = status
        self._err_message = msg
        self._index_content = json.loads(index_content.decode("utf-8"))

    def get_status(self):
        return self._status

    def get_index_content(self):
        return self._index_content

    def get_value(self, key):
        json_value = json.loads(_index_content[key].decode("utf-8"))
        return json_value["obj-name"]

    def get_error(self):
        return self._err_message