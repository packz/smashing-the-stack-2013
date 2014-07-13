CPPFLAGS=-Wall -m32 -pie -fstack-protector-all
LDFLAGS=-Wl,-z,relro,-z,now

BINARY=server exploit injecter vuln
vuln: CPPFLAGS=-Wall -m32 -fno-stack-protector -g

all: $(BINARY)
clean:
	rm -vf $(BINARY)
