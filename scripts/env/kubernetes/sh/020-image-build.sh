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

set -euo pipefail # exit on failures

source ./config.sh
source ./env.sh
source ./sh/functions.sh

set -x # print each statement before execution

# See if we even asked to build image.
if [ -z "$S3_CORTX_ALL_CUSTOM_CI_NUMBER" ]; then
  # no need to build image. cleanup env.sh
  sed -e '/^S3_CORTX_ALL_IMAGE_TAG=/d' -i env.sh
  exit 0
fi

new_image_tag="2.0.0-${S3_CORTX_ALL_CUSTOM_CI_NUMBER}-custom-ci"

# see if maybe image is already built?
if [ -n "$(docker images ghcr.io/seagate/cortx-all | safe_grep "$new_image_tag")" ]; then
  # image already built
  exit 0
fi

# see if maybe image already available on github container registry?
if docker pull "ghcr.io/seagate/cortx-all:$new_image_tag"; then
  # available
  exit 0
fi

add_separator BUILDING CORTX-ALL IMAGE LOCALLY.

if ! which docker-compose; then
  sudo curl -L "https://github.com/docker/compose/releases/download/1.29.2/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose

  sudo chmod +x /usr/local/bin/docker-compose

  sudo ln -s /usr/local/bin/docker-compose /usr/bin/docker-compose

  docker-compose --version
fi

if [ ! -d cortx-re ]; then
  echo -ne "\n\n\nNOTE!\n\nGihub will ask for authorization below. Create token, as documented here:\n\nhttps://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token\n\nAnd then use this token instead of password in the prompt.\n\n\n"
  git clone https://github.com/seagate/cortx-re -b kubernetes
fi

( set -x -e
  cd cortx-re/docker/cortx-deploy
  ./build.sh -b custom-build-"$S3_CORTX_ALL_CUSTOM_CI_NUMBER"
)

echo "S3_CORTX_ALL_IMAGE_TAG='$new_image_tag'" >> env.sh

add_separator SUCCESSFULLY BUILT CORTX-ALL IMAGE LOCALLY.
