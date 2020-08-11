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

ARG  CENTOS_RELEASE=7
FROM registry.gitlab.mero.colo.seagate.com/motr/motr:${CENTOS_RELEASE}

COPY s3server-uploads.repo /etc/yum.repos.d/
COPY s3rpm.spec .

RUN yum install -y yum-priorities

RUN yum install -y http://cortx-storage.colo.seagate.com/releases/master/mero_stub/mero-0.0.0-stub.x86_64.rpm \
                   http://cortx-storage.colo.seagate.com/releases/master/mero_stub/mero-devel-0.0.0-stub.x86_64.rpm \
    && yum install -y git-clang-format \
    && yum-builddep -y --define 'python3_other_pkgversion 36' s3rpm.spec \
    && rm -f s3rpm.spec \
    && yum remove -y cortx-motr cortx-motr-devel
