CPPFLAGS=-Wall -m32 -pie -fstack-protector-all
LDFLAGS=-Wl,-z,relro,-z,now

BINARY=server exploit

all: $(BINARY)
clean:
	rm -vf $(BINARY)
