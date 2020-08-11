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

"""CORTXS3ListIndexResponse will list out index response."""
import json


class CORTXS3ListIndexResponse(object):

    """CORTXS3ListIndexResponse will list index response."""
    _index_content = ""

    def __init__(self, index_content):
        """Initialise index content."""
        self._index_content = json.loads(index_content.decode("utf-8"))

    def get_index_content(self):
        """return index content."""
        return self._index_content

    def get_json(self, key):
        """Returns the content value based on key."""
        json_value = json.loads(self._index_content[key].decode("utf-8"))
        return json_value

    def set_index_content(self, index_content):
        """Sets index content."""
        self._index_content = json.loads(index_content.decode("utf-8"))
