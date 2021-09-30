#!/bin/bash
#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

set -e -x

s3_repo_dir=/var/data/cortx/cortx-s3server
src_dir="$s3_repo_dir"/scripts/env/kubernetes

source "$src_dir/env.sh"

MACHINE_ID=211072f61c4b4949839c624d6ed95115

echo "$MACHINE_ID" > /etc/machine-id

config_tmpl=/opt/seagate/cortx/s3/conf/s3.config.tmpl.1-node
init_tmpl=/opt/seagate/cortx/s3/conf/s3.init.tmpl.1-node

for f in "$config_tmpl" "$init_tmpl"; do
  sed -i \
    -e "s/TMPL_ROOT_SECRET_KEY/gAAAAABhQHB7hl-zHPmL3fcNWB9qXjZIWXAAH8uPec3ZuTo2JLRHdJPYDrbYBW90mPF3NmbSAZhQxnclbkwNEMLmINLx-JULUw==/g" \
    -e "s/TMPL_SGIAM_SECRET_KEY/gAAAAABhQHB7hl-zHPmL3fcNWB9qXjZIWXAAH8uPec3ZuTo2JLRHdJPYDrbYBW90mPF3NmbSAZhQxnclbkwNEMLmINLx-JULUw==/g" \
    -e "s/TMPL_CLUSTER_ID/3f670dd0-17cf-4ef3-9d8b-e1fb6a14c0f6/g" \
    -e "s/TMPL_MACHINE_ID/$MACHINE_ID/g" \
    -e "s/TMPL_HOSTNAME/s3-setup-pod/g" \
    "$f"
  # Update LDAP endpoints; not easy to do with SED.
  python3 <<EOF
#!/usr/bin/python3

import yaml
import urllib.parse

with open("$f") as f:
  y = yaml.safe_load(f)
  lst = y['cortx']['external']['openldap']['endpoints']
  lnew = []
  for ep in lst:
    result1 = urllib.parse.urlparse(ep)
    result2 = result1.netloc.split(':')
    result = { 'scheme': result1.scheme, 'fqdn': result2[0] }
    ep_new = result1.scheme + '://${OPENLDAP_SVC}:' + result2[1]
    lnew.append(ep_new)
  y['cortx']['external']['openldap']['endpoints'] = lnew
with open("$f", 'w') as f:
  f.write(yaml.dump(y, default_flow_style=False, sort_keys=False))
EOF
done


/opt/seagate/cortx/s3/bin/s3_setup post_install --config "yaml:///opt/seagate/cortx/s3/conf/s3.post_install.tmpl.1-node" --services=io
/opt/seagate/cortx/s3/bin/s3_setup prepare      --config "yaml:///opt/seagate/cortx/s3/conf/s3.prepare.tmpl.1-node"      --services=io
/opt/seagate/cortx/s3/bin/s3_setup config       --config "yaml:///opt/seagate/cortx/s3/conf/s3.config.tmpl.1-node"       --services=io
/opt/seagate/cortx/s3/bin/s3_setup init         --config "yaml:///opt/seagate/cortx/s3/conf/s3.init.tmpl.1-node"         --services=io

