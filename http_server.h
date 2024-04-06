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

void http_header_print(const HTTPHeader *header);
void http_header_free(HTTPHeader *header);
HTTPHeader *http_header_new(char *name, char *value);

void http_request_line_print(const HTTPRequestLine *request_line);
void http_request_line_free(HTTPRequestLine *request_line);
HTTPRequestLine *http_request_line_new(char *method, char *uri, char *version);

#define REGEX_REQUEST_LINE "^(GET|HEAD|POST) (/.*) HTTP/([0-9]+\\.[0-9]+)" CRLF
#define REGEX_HEADER       "^([^:]+): (.*?)" CRLF

#endif
