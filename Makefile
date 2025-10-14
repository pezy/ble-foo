IMAGE := armforge

.PHONY: docker sh build

docker:
	@docker buildx build --platform linux/arm64 -t $(IMAGE) -f Dockerfile .

sh:
	@docker run -it --rm -v $(PWD):/ws -w /ws $(IMAGE) bash

build:
	@docker run --rm -v $(PWD):/ws -w /ws $(IMAGE) sh -c "cmake -S . -B build && cmake --build build"

deploy:
	scp build/ble_paired build/libble.so.1 build/libble.so.1.0.0 x5:/app/ble/

b:
	@cmake -S . -B build && cmake --build build
