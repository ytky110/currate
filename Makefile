bin/currate: src/currate.c
	cc $^ -o $@ -lcurl -ljson-c

.PHONY: clean

clean:
	rm -f bin/*
