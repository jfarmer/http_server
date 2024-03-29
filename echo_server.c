#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

#define BUFFER_SIZE 50
#define LISTEN_BACKLOG 8

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int bind_server_socket(int port) {
  int server_fd, err;
  struct sockaddr_in server;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    return server_fd;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    close(server_fd);
    return err;
  }

  return server_fd;
}

int echo_to_client(int client_fd, void *buffer, size_t buffer_length) {
  while (1) {
    int bytes_sent, bytes_recvd;

    bytes_recvd = recv(client_fd, buffer, buffer_length, 0);

    printf("server: read %d bytes\n", bytes_recvd);

    if (bytes_recvd == 0) {
      // Connection closed in an orderly fashion, so return
      return 0;
    }

    if (bytes_recvd < 0) {
      return bytes_recvd;
    }

    bytes_sent = send(client_fd, buffer, bytes_recvd, 0);
    if (bytes_sent < 0) {
      return bytes_sent;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    fflush(stderr);
    return EXIT_FAILURE;
  }

  int port = atoi(argv[1]);

  int server_fd, err;
  char net_address[INET6_ADDRSTRLEN];
  char network_buffer[BUFFER_SIZE];

  server_fd = bind_server_socket(port);
  if (server_fd < 0) {
    perror("Unable to bind to socket");
    exit(EXIT_FAILURE);
  }

  err = listen(server_fd, LISTEN_BACKLOG);
  if (err < 0) {
    perror("Unable to bind to socket");
    exit(EXIT_FAILURE);
  }

  printf("server: listening on port %d\n", port);

  while (1) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      perror("server: error establishing new connection");
      continue;
    }

    inet_ntop(client.sin_family, get_in_addr((struct sockaddr *)&client),
              net_address, sizeof net_address);
    printf("server: new connection from %s\n", net_address);

    err = echo_to_client(client_fd, network_buffer, BUFFER_SIZE);
    close(client_fd);

    if (err < 0) {
      perror("server: client error");
    } else {
      printf("server: connection closed from %s\n", net_address);
    }
  }

  return EXIT_SUCCESS;
}
