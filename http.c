#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_server.h"

void http_header_print(const HTTPHeader *header) {
  printf("Name:    %s\n", header->name);
  printf("Value:   %s\n", header->value);
}

void http_header_free(HTTPHeader *header) {
  free(header->name);
  free(header->value);
  free(header);
}

HTTPHeader *http_header_new(char *name, char *value) {
  HTTPHeader *header = malloc(sizeof(HTTPHeader));
  if (!header) {
    perror("Failed to allocate memory for HTTPHeader");
    return NULL;
  }

  header->name  = name;
  header->value = value;

  return header;
}

void http_request_line_print(const HTTPRequestLine *request_line) {
  printf("Method:  %s\n", request_line->method);
  printf("URI:     %s\n", request_line->uri);
  printf("Version: %s\n", request_line->version);
}

void http_request_line_free(HTTPRequestLine *request_line) {
  free(request_line->method);
  free(request_line->uri);
  free(request_line->version);
  free(request_line);
}

HTTPRequestLine *http_request_line_new(char *method, char *uri, char *version) {
  HTTPRequestLine *request_line = malloc(sizeof(HTTPRequestLine));
  if (!request_line) {
    perror("Failed to allocate memory for HTTPRequestLine");
    return NULL;
  }

  request_line->method  = method;
  request_line->uri     = uri;
  request_line->version = version;

  return request_line;
}
