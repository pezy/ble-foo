IMAGE := armforge

.PHONY: docker sh

docker:
	@docker buildx build --platform linux/arm64 -t $(IMAGE) -f Dockerfile .

sh: docker
	@docker run -it --rm -v $(PWD):/ws -w /ws $(IMAGE) bash
