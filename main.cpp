#include <arpa/inet.h>
#include <iostream>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVERPORT 5000
#define BUFSIZE 4096
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 1

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void handle_connection(int client_socket);
int check(int exp, const char *msg);

int main() {
  int server_socket, client_socket, addr_size;
  SA_IN server_addr, client_addr;

  check((server_socket = socket(AF_INET, SOCK_STREAM, 0)),
        "failed to create socket");

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVERPORT);

  check(bind(server_socket, (SA *)&server_addr, sizeof(server_addr)),
        "Bind Failed");
  check(listen(server_socket, SERVER_BACKLOG), "Listen Failed");

  while (true) {
    printf("waiting for connections...\n");
    fflush(stdout);
    addr_size = sizeof(SA_IN);
    check(client_socket = accept(server_socket, (SA *)&client_addr,
                                 (socklen_t *)&addr_size),
          "accept field");
    printf("Connected!\n");
    handle_connection(client_socket);
  }
  return 0;
}

int check(int exp, const char *msg) {
  if (exp == SOCKETERROR) {
    perror(msg);
    exit(1);
  }
  return exp;
}

void handle_connection(int client_socket) {
  char buffer[BUFSIZE];
  size_t bytes_read;
  int msgsize = 0;
  char actualpath[PATH_MAX + 1];

  while ((bytes_read = read(client_socket, buffer + msgsize,
                            sizeof(buffer) - msgsize - 1)) > 0) {
    msgsize += bytes_read;
    if (msgsize > BUFSIZE - 1 || buffer[msgsize - 1] == '\n')
      break;
  }
  check(bytes_read, "recv error");
  buffer[msgsize - 1] = 0;

  printf("REQUEST: %s\n", buffer);

  // realpath "/home/alex/build-tsp_server-Desktop-Debug"
  char *fullpath = realpath(*&buffer, *&actualpath);

  fflush(stdout);

  if (fullpath == NULL) {
    printf("ERROR(bad path): %s\n", buffer);
    printf("actualpath: %s\n", actualpath);
    printf("fullpath: %s\n", fullpath);
    close(client_socket);
    return;
  }

  FILE *fp = fopen(actualpath, "r");
  if (fp == NULL) {
    printf("ERROR(open): %s\n", buffer);
    close(client_socket);
    return;
  }

  while ((bytes_read = fread(buffer, 1, BUFSIZE, fp)) > 0) {
    printf("sending %zu bytes\n", bytes_read);
    write(client_socket, buffer, bytes_read);
  }

  close(client_socket);
  fclose(fp);
  printf("closing connection\n");
}