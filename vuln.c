#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

void check(char* src) {
    char buffer[16];

    memcpy(buffer, src, strlen(src));
}

int main() {
    char buffer[256];

    printf("Please enter your code: ");
    fflush(stdout);

    int count = read(0, buffer, 256);
    buffer[count] = '\0';

    check(buffer);

    printf("login failed\n");

    return 0;
}
