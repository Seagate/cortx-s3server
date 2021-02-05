#!/usr/bin/python3.6

import sys
import os
from s3backgrounddelete.cortx_s3_count_kv import CORTXS3Countkv

if __name__ == "__main__":
    obj = CORTXS3Countkv()
    if len(sys.argv) == 2:
        obj.count(sys.argv[1])
    elif len(sys.argv) > 2:
        obj.count(sys.argv[1], sys.argv[2])
    else:
        obj.count()
