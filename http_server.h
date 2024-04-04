#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

typedef struct _HTTPHeader {
  char *name;
  char *value;
} HTTPHeader;

typedef struct _HTTPRequestLine {
  char *method;
  char *uri;
  char *version;
} HTTPRequestLine;

typedef struct _HTTPEntityBody {
  char *content;
} HTTPEntityBody;

typedef struct _HTTPRequest {
  HTTPRequestLine *request_line;
  HTTPHeader **headers;
  HTTPEntityBody *body;
} HTTPRequest;

#define REGEX_REQUEST_LINE "^(GET|HEAD|POST) (/.*) HTTP/([0-9]+\\.[0-9]+)" CRLF
#define REGEX_HEADER       "^([^:]+): (.*?)" CRLF

#endif
