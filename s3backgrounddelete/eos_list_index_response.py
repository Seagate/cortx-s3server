import json

class EOSCoreListIndexResponse:
    _index_content = ""

    def __init__(self, index_content):
        self._index_content = json.loads(index_content.decode("utf-8"))

    def get_index_content(self):
        return self._index_content

    def get_json(self, key):
        json_value = json.loads(_index_content[key].decode("utf-8"))
        return json_value
