IMAGE := armforge

.PHONY: docker sh build deploy b f l

docker:
	@docker buildx build --platform linux/arm64 -t $(IMAGE) -f Dockerfile .

sh:
	@docker run -it --rm -v $(PWD):/ws -w /ws $(IMAGE) bash

build:
	@docker run --rm -v $(PWD):/ws -w /ws $(IMAGE) sh -c "cmake -S . -B build-arm64 && cmake --build build-arm64"

deploy:
	scp build/ble_pair build/libble.so.1 build/libble.so.1.0.0 x5:/app/ble/

b:
	@cmake -S . -B build && cmake --build build

f:
	cd build && make format

l:
	cd build && make cpplint