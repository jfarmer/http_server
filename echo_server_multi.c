#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#include <unistd.h>

#define BUFFER_SIZE 50
#define LISTEN_BACKLOG 8

void handler_sigchld(int s) {
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

void setup_signal_handlers() {
  struct sigaction sigchild_action;

  sigchild_action.sa_handler = handler_sigchld;
	sigemptyset(&sigchild_action.sa_mask);

	sigchild_action.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sigchild_action, NULL) == -1) {
		perror("Error setting up SIGCHLD handler");
		exit(EXIT_FAILURE);
	}
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

  err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
  if (err < 0) {
    close(server_fd);
    return err;
  }

  return server_fd;
}

int echo_to_client(int client_fd, void *buffer, size_t buffer_length) {
  while(1) {
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

  /*
    To see what difference handling SIGCHILD makes, do the following:

    1. Comment out setup_signal_handlers() so we don't call it
    2. Run the server on port 3490
    3. Run "echo hello | nc localhost 3490" three or four times
    4. Run "ps aux | grep echo"

    Notice that there are still echo_server processes hanging around even though
    the clients have all disconnected. These are so-called "zombie processes" and
    their presence is a symptom of using fork() without handling SIGCHILD properly.
  */
  setup_signal_handlers();

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

    int client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);

    if (client_fd < 0) {
      perror("server: error establishing new connection");
      continue;
    }

    inet_ntop(client.sin_family, get_in_addr((struct sockaddr *) &client), net_address, sizeof net_address);
		printf("server: new connection from %s\n", net_address);

    // fork() clones the current process
    // it returns 0 to the child process and the process ID of the child to the parent process
    pid_t pid = fork();
    if (pid < 0) {
      // error forking
      perror("server: error forking server process");
      exit(EXIT_FAILURE);
    }

    if (pid > 0) {
      // we are in the parent process, but only child needs client_fd

      // do nothing else and go back to waiting for next connection
      close(client_fd);
    } else {
      // we are in the child process, but only parent needs server_fd
      close(server_fd);

      err = echo_to_client(client_fd, network_buffer, BUFFER_SIZE);
      if (err < 0) {
        perror("server: client error");
        exit(EXIT_FAILURE);
      }

      close(client_fd);

      printf("server: connection closed from %s\n", net_address);

      // exit() here exits from the child process
      exit(EXIT_SUCCESS);
    }
  }

  return EXIT_SUCCESS;
}
