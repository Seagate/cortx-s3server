#!/bin/sh
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

set -e

GITHUB_TOKEN=""
TAG_NAME=""

usage() { echo "Usage: $0 [-G git_access_token ]" 1>&2; exit 1; }

if [[ $# -eq 0 ]] ; then
    usage
fi

OPTIND=1

while getopts "G:" x; do
    case "${x}" in
        G)
            GITHUB_TOKEN=${OPTARG}
            [ -z "${GITHUB_TOKEN}" ] && (echo "Git access token cannot be empty"; usage)
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

pip install githubrelease

# Clean existing cache
rm -rf /root/releases_cortx_s3deps /etc/yum.repos.d/releases_cortx_s3deps.repo

mkdir -p /root/releases_cortx_s3deps
cd /root/releases_cortx_s3deps

# Download CORTX dependencies rpm
TAG_NAME="build-dependencies"
githubrelease --github-token "$GITHUB_TOKEN" asset seagate/cortx download "${TAG_NAME}"

cd -

cat <<EOF >/etc/yum.repos.d/releases_cortx_s3deps.repo
[releases_cortx_s3deps]
name=Cortx-S3 Repository
baseurl=file:///root/releases_cortx_s3deps
gpgcheck=0
enabled=1
EOF

createrepo -v /root/releases_cortx_s3deps
yum clean all
yum repolist
