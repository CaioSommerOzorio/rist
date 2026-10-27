.PHONY: build

build:
	mkdir -p build
	g++ src/main.cpp -o build/rist

clean:
	rm -rf build
