#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include "http_server.h"

#include <regex.h>

regex_t REGEX_T_REQUEST_LINE;
regex_t REGEX_T_HEADER;

#define CRLF "\r\n"
#define CRLFCRLF "\r\n\r\n"
#define BUFFER_SIZE 8 * 1024 // 8 kB
#define LISTEN_BACKLOG 256

void handler_sigchld(int s) {
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0);

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

void setup_regex() {
  if (regcomp(&REGEX_T_REQUEST_LINE, REGEX_REQUEST_LINE,
              REG_EXTENDED | REG_ENHANCED) != 0) {
    perror("Failed to compile REGEX_REQUEST_LINE");
    exit(EXIT_FAILURE);
  }

  if (regcomp(&REGEX_T_HEADER, REGEX_HEADER, REG_EXTENDED | REG_ENHANCED) !=
      0) {
    perror("Failed to compile REGEX_HEADER");
    exit(EXIT_FAILURE);
  }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

HTTPRequestLine *http_parse_request_line(char *request, int *offset) {
  const char *method_loc, *uri_loc, *version_loc;
  char *method, *uri, *version;
  int offset2 = 0;

  method_loc = strchr(request, ' ');
  if (method_loc == NULL) {
    return NULL;
  }

  offset2 += method_loc - request + 1;

  uri_loc = strchr(method_loc + 1, ' ');
  if (uri_loc == NULL) {
    return NULL;
  }

  offset2 += uri_loc - (method_loc + 1) + 1;

  version_loc = strstr(uri_loc + 1, "\r\n");
  if (version_loc == NULL) {
    return NULL;
  }

  offset2 += version_loc - (uri_loc + 1) + 2;

  method = strndup(request, method_loc - request);
  uri = strndup(method_loc + 1, uri_loc - (method_loc + 1));
  version = strndup(uri_loc + 1, version_loc - (uri_loc + 1));

  // regmatch_t matches[4];

  // if (regexec(&REGEX_T_REQUEST_LINE, request + *offset, 4, matches, 0) != 0)
  // {
  //   return NULL;
  // }

  *offset += offset2;

  // printf("real offset: %d\n", *offset);
  // printf("new offset:  %d\n", offset2);

  return http_request_line_new(method, uri, version);

  // return http_request_line_new(
  //   strndup(request + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so),
  //   strndup(request + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so),
  //   strndup(request + matches[3].rm_so, matches[3].rm_eo - matches[3].rm_so)
  // );
}

HTTPHeader *http_parse_header(char *request, int *offset) {
  char *offset_request = request + *offset;
  const char *header_delim_loc, *header_crlf_loc;
  int offset2 = 0;
  // regmatch_t matches[3];

  // if (regexec(&REGEX_T_HEADER, offset_request, 3, matches, 0) != 0) {
  //   return NULL;
  // }

  header_delim_loc = strstr(offset_request, ": ");
  if (!header_delim_loc) {
    return NULL;
  }

  offset2 += header_delim_loc - offset_request + 2;

  header_crlf_loc = strstr(header_delim_loc + 2, "\r\n");
  if (!header_crlf_loc) {
    return NULL;
  }

  offset2 += header_crlf_loc - (header_delim_loc + 2) + 2;

  *offset += offset2;

  return http_header_new(
      strndup(offset_request, header_delim_loc - offset_request),
      strndup(header_delim_loc + 2, header_crlf_loc - (header_delim_loc + 2)));
  // *offset += matches[0].rm_eo;

  // return http_header_new(
  //   strndup(offset_request + matches[1].rm_so, matches[1].rm_eo -
  //   matches[1].rm_so), strndup(offset_request + matches[2].rm_so,
  //   matches[2].rm_eo - matches[2].rm_so)
  // );
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

int handle_http_request(int client_fd, void *buffer, size_t buffer_length) {
  HTTPRequestLine *request_line = NULL;
  HTTPHeader *header = NULL;

  while (1) {
    int bytes_sent, bytes_recvd;

    bytes_recvd = recv(client_fd, buffer, buffer_length, 0);

    if (bytes_recvd == 0) {
      // Connection closed in an orderly fashion, so return
      return 0;
    }

    if (bytes_recvd < 0) {
      return bytes_recvd;
    }

    int offset = 0;

    request_line = http_parse_request_line(buffer, &offset);
    if (request_line) {
      // http_request_line_print(request_line);
      http_request_line_free(request_line);
    }

    while ((header = http_parse_header(buffer, &offset)) != NULL) {
      // http_header_print(header);
      http_header_free(header);
    }

    // printf("All done w/ headers!\n");

    const char body[] = "Hello, World!";
    const char response[] =
        "HTTP/1.0 200 OK" CRLF "Content-Length: %d" CRLF CRLF "%s";

    int content_length = strlen(body);
    int digits_in_content_length = snprintf(NULL, 0, "%d", content_length);
    int full_response_length =
        strlen(response) + digits_in_content_length + content_length + 1;

    char *full_response = calloc(full_response_length, sizeof(char));

    int response_length = snprintf(full_response, full_response_length,
                                   response, content_length, body);

    bytes_sent = send(client_fd, full_response, response_length, 0);
    if (bytes_sent < 0) {
      return bytes_sent;
    }

    break;
  }

  return 0;
}

void handle_client_disconnect(int client_fd) {
  close(client_fd);
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

  int kq;
  struct kevent change_event[4], event[4];

  setup_signal_handlers();
  setup_regex();

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
  kq = kqueue();
  EV_SET(change_event, server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);

  if (kevent(kq, change_event, 1, event, 0, NULL) == -1) {
    perror("kevent");
    exit(EXIT_FAILURE);
  }

  while (1) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    int new_events = kevent(kq, NULL, 0, event, 4, NULL);

    if (new_events < 0) {
      perror("kevent");
      exit(EXIT_FAILURE);
    }

    printf("new events: %d\n", new_events);

    for (int i = 0; i < new_events; i++) {
      int event_fd = event[i].ident;

      if (event[i].flags & EV_EOF) {
        // printf("server: client disconnected\n");
        handle_client_disconnect(event_fd);
      } else if (event_fd == server_fd) {
        // printf("server: client connected\n");
        int client_fd =
            accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
          perror("server: error establishing new connection");
        }

        // inet_ntop(client.sin_family, get_in_addr((struct sockaddr *)&client),
        //           net_address, sizeof net_address);
        // printf("server: new connection from %s\n", net_address);

        EV_SET(change_event, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

        if (kevent(kq, change_event, 1, NULL, 0, NULL) < 0) {
          perror("kevent error");
        }
      } else if (event[i].filter & EVFILT_READ) {
        char network_buffer[BUFFER_SIZE];

        err = handle_http_request(event_fd, network_buffer, BUFFER_SIZE);
        if (err < 0) {
          perror("server: client error");
          exit(EXIT_FAILURE);
        }

        close(event_fd);
      } else {
        printf("Unknown event\n");
      }
    }
  }

  return EXIT_SUCCESS;
}
