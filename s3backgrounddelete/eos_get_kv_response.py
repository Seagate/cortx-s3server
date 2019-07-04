import json

class EOSCoreGetKVResponse:
    _key = ""
    _value = ""

    def __init__(self, key, value):
        self._key = key
        self._value = value.decode("utf-8")

    def get_key(self):
        return self._key

    def get_value(self):
        return self._value
