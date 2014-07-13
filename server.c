/*
* server - a simple server vulnerable to stack overflow
*
* Copyright 2012-2013 Benjamin Randazzo <benjamin@linuxcrashing.org>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#define KEY 0x42
#define SECRET "\x73\x71\x71\x75"

#define PORT 3333

void encrypt(char *buffer, int key) {
    int len, i;
    len = strlen(SECRET);
    for (i = 0 ; i < len ; i++)
        buffer[i] ^= key;
}
int check_code(char *data, int size) {
    char buffer[20];
    memcpy(buffer, data, size);
    encrypt(buffer, KEY);
    return strncmp(buffer, SECRET, strlen(SECRET));
}
int main() {
    char buffer[512];
    int sockfd, sock_newfd, optval = 1;
    struct sockaddr_in addr, serv_addr;
    int sin_size = sizeof(struct sockaddr_in), bsize;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(errno);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) { 
        perror("setsockopt");
        exit(errno);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), 0, 8);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) { 
        perror("bind");
        exit(errno);
    }

    if (listen(sockfd, 20) < 0) {
        perror("listen");
        exit(errno);
    }

    printf("ready\n");

    while (1) {
        if ((sock_newfd = accept(sockfd, (struct sockaddr *)&addr, &sin_size)) < 0) {
            perror("accept");
            exit(errno);
        }

        if (fork() == 0) {

            write(sock_newfd, "Bank of France\n", 15);
            write(sock_newfd, "Enter code : ", 13);
            bzero(buffer, sizeof(buffer));
            bsize = read(sock_newfd, buffer, sizeof(buffer), 0);
            if (check_code(buffer, bsize) == 0) {
                write(sock_newfd, "\n=== Access granted ===\n", 24);
            } else {
                write(sock_newfd, "\nAccess denied\n", 15);
            }
            close(sock_newfd);
            exit(0);
        } 
        close(sock_newfd);
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }
    close(sockfd);
    return 0;
}

