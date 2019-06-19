import json

class EOSCoreKVResponse:
    _status = 200
    _err_message = ""
    _key = ""
    _value = ""

    def __init__(self, status, msg, key, value):
        self._status = status
        self._err_message = msg
        self._key = key
        self._value = value

    def get_status(self):
        return self._status

    def get_key(self):
        return self._key

    def get_value(self):
        return self._value

    def get_error(self):
        return self._err_message

    def get_json_parsed_value(self):
        json_value = json.loads(self._value)
        return json_value["obj-name"]