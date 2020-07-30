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

attr={ "name": "s3_req" }
def query(from_, to_):
    q=f"""
    SELECT (s3r.time-s3_request_state.time) as time, s3_request_state.state, s3r.state, s3r.s3_request_id as id FROM s3_request_state
    JOIN s3_request_state s3r ON s3r.s3_request_id=s3_request_state.s3_request_id
    WHERE s3_request_state.state="{from_}"
    AND s3r.state="{to_}";
    """
    return q

if __name__ == '__main__':
    import sys
    sys.exit(1)
