bin/currate: src/currate.c
	mkdir -p bin
	cc $^ -o $@ -lcurl -ljson-c

.PHONY: clean

clean:
	rm -f bin/*
