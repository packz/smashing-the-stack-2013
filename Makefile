CPPFLAGS=-Wall -m32 -pie -fstack-protector-all
LDFLAGS=-Wl,-z,relro,-z,now

# seems that gcc-4.9 has more check
CC=gcc-4.8

BINARY=server exploit injecter vuln
vuln: CPPFLAGS=-Wall -m32 -fno-stack-protector -g

all: $(BINARY)
clean:
	rm -vf $(BINARY)
