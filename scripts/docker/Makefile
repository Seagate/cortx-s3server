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

CENTOS_RELEASE  := 7
NAMESPACE       := registry.gitlab.mero.colo.seagate.com
DOCKER          := docker

INAME = $(@:%-image=%)
CNAME = $(@:%-container=%)

.PHONY: images
images: docker-image-latest \
        docker-image-7.6 \
        docker-image-cortx

.PHONY: docker-image-latest docker-image-7
docker-image-latest docker-image-7: s3server-devel-image

.PHONY: docker-image-7.6
docker-image-7.6: CENTOS_RELEASE := 7.6
docker-image-7.6: s3server-devel-image

.PHONY: docker-image-cortx
docker-image-cortx: CENTOS_RELEASE := cortx
docker-image-cortx: s3server-devel-image


.PHONY: s3server-devel-image
s3server-devel-image:
	cd $(INAME) \
	&& tar -ch . \
	   | $(DOCKER) build \
			--build-arg CENTOS_RELEASE=$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/$(INAME):$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/motr/s3server:$(CENTOS_RELEASE) \
			$(DOCKER_OPTS) -

.PHONY: push
name := s3server*
tag  := *
push:
	@for img in $$(docker images --filter=reference='$(NAMESPACE)/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}' | grep -v none) \
		    $$(docker images --filter=reference='$(NAMESPACE)/motr/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}' | grep -v none) ; \
	do \
		echo "---> $$img" ; \
		$(DOCKER) push $$img ; \
	done

.PHONY: clean
clean:
	@for img in $$(docker images --filter=reference='$(NAMESPACE)/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}') \
		    $$(docker images --filter=reference='$(NAMESPACE)/motr/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}') ; \
	do \
		echo "---> $$img" ; \
		$(DOCKER) rmi $$img ; \
	done

.PHONY: help
help:
	@echo 'Build targets:'
	@echo '  images          - create docker images for CI environment'
	@echo ''
	@echo 'Publish targets:'
	@echo "  push            - upload local $(NAMESPACE)/* images to docker hub repository"
	@echo ''
	@echo 'Clean targets:'
	@echo "  clean           - remove local $(NAMESPACE)/* images"
