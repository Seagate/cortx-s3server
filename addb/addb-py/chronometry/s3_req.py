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

import peewee
from addb2db import *
from playhouse.shortcuts import model_to_dict
import matplotlib.pyplot as plt

def query2dlist(model):
    out=[]
    for m in model:
        out.append(model_to_dict(m))
    return out

def draw_timeline(timeline, offset):
    for i in range(len(timeline)-1):
        color = ['red', 'green', 'blue', 'yellow', 'magenta'][i%5]
        start  = timeline[i]['time']
        end    = timeline[i+1]['time']
        label  = timeline[i]['state']

        plt.hlines(offset, start, end, colors=color, lw=5)
        plt.text(start, offset, label, rotation=90)

    end    = timeline[-1]['time']
    label  = timeline[-1]['state']
    plt.text(end, offset, label, rotation=90)

    label  = timeline[0]['label']
    plt.text(end, offset, label)


def main():
    parser = argparse.ArgumentParser(description="draws s3 request timeline")
    parser.add_argument('--s3reqs', nargs='+', type=int, required=True,
                        help="requests ids to draw")
    parser.add_argument('--db', type=str, required=False, default="m0play.db",
                        help="input database file")
    parser.add_argument('--no_motr', action='store_true', required=False,
                        default=False, help="exclude motr requests from timeline")
    args = parser.parse_args()

    db_init(args.db)
    db_connect()

    time_table=[]
    ref_time = []

    for s3id in args.s3reqs:
        s3_req_rel = s3_request_uid.select().where(s3_request_uid.s3_request_id == s3id).get()
        s3_req_d = query2dlist(s3_request_state.select().where(s3_request_state.s3_request_id == s3_req_rel.s3_request_id))
        l = "     {}".format(s3_req_rel.uid)
        for r in s3_req_d:
            r['label'] = l

        ref_time.append(min([t['time'] for t in s3_req_d]))
        time_table.append(s3_req_d)

        if not args.no_motr:
            s3_to_motr_d = query2dlist(s3_request_to_motr.select().where(s3_request_to_motr.s3_request_id == s3_req_rel.s3_request_id))

            for s3c in s3_to_motr_d:
                motr = query2dlist(motr_req.select().where(motr_req.id == s3c['motr_id']))
                l = "      {}".format(s3c['motr_id'])
                for c in motr:
                    c['label'] = l
                time_table.append(motr)

    db_close()

    min_ref_time = min(ref_time)
    for times in time_table:
        for time in times:
            time['time']=time['time'] - min_ref_time

    for times in time_table:
        times.sort(key=lambda time: time['time'])

    offset=1
    for times in time_table:
        draw_timeline(times, offset)
        offset=offset-1

    plt.grid(True)
    plt.show()


main()
