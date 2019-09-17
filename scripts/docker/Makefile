CENTOS_RELEASE  := latest
NAMESPACE       := registry.gitlab.mero.colo.seagate.com
DOCKER          := docker

INAME = $(@:%-image=%)
CNAME = $(@:%-container=%)

.PHONY: docker-images
docker-images: docker-image-latest \
               docker-image-7.5 \
               docker-image-eos

.PHONY: docker-image-latest
docker-image-latest: s3server-devel-image

.PHONY: docker-image-7.5
docker-image-7.5: CENTOS_RELEASE := 7.5
docker-image-7.5: override DOCKER_OPTS += --build-arg CENTOS_RELEASE=$(CENTOS_RELEASE)
docker-image-7.5: s3server-devel-image

.PHONY: docker-image-eos
docker-image-eos: CENTOS_RELEASE := eos
docker-image-eos: override DOCKER_OPTS += --build-arg CENTOS_RELEASE=$(CENTOS_RELEASE)
docker-image-eos: s3server-devel-image


.PHONY: s3server-devel-image
s3server-devel-image:
	cd $(INAME) \
	&& tar -ch . \
	   | $(DOCKER) build \
			-t $(NAMESPACE)/$(INAME):$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/mero/s3server:$(CENTOS_RELEASE) \
			$(DOCKER_OPTS) -

name := s3server*
tag  := *
docker-push:
	@for img in $$(docker images --filter=reference='$(NAMESPACE)/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}' | grep -v none) \
		    $$(docker images --filter=reference='$(NAMESPACE)/mero/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}' | grep -v none) ; \
	do \
		echo "---> $$img" ; \
		$(DOCKER) push $$img ; \
	done

docker-clean:
	@for img in $$(docker images --filter=reference='$(NAMESPACE)/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}') \
		    $$(docker images --filter=reference='$(NAMESPACE)/mero/$(name):$(tag)' \
				    --format '{{.Repository}}:{{.Tag}}') ; \
	do \
		echo "---> $$img" ; \
		$(DOCKER) rmi $$img ; \
	done

help:
	@echo 'Build targets:'
	@echo '  docker-images   - create docker images for CI environment'
	@echo "  docker-push     - upload local $(NAMESPACE)/* images to docker hub repository"
	@echo "  docker-clean    - remove local $(NAMESPACE)/* images"
	@echo ''
	@echo 'Publish targets:'
	@echo "  push            - upload local $(NAMESPACE)/* images to docker hub repository"
	@echo ''
	@echo 'Clean targets:'
	@echo "  clean           - remove local $(NAMESPACE)/* images"
