#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>


#define BIND_PORT "3490"
#define LISTEN_BACKLOG 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main() {
  int addr_status, sockfd, bind_status, new_fd, msg_len, bytes_sent;
	struct addrinfo hints, *servinfo, *p;;
	struct sockaddr_storage their_addr;
	char *msg = "Hi I'm a datetime	";
	socklen_t addr_size;
	char s[INET6_ADDRSTRLEN];

	// get local addr
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((addr_status = getaddrinfo(NULL, BIND_PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(addr_status));
      exit(1);
  }
	// check for valid results?

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		// if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
		// 		sizeof(int)) == -1) {
		// 	perror("setsockopt");
		// 	exit(1);
		// }

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	// // get socket
	// if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) !=0) {
	// 	fprintf(stderr, "socket error: %s\n", gai_strerror(sockfd));
  //   exit(1);
	// };

	// // bind to socket
	// if ((bind_status = bind(sockfd, res->ai_addr, res->ai_addrlen)) !=0) {
	// 	fprintf(stderr, "bind error: %s\n", gai_strerror(bind_status));
  //   exit(1);
	// }

	// listen
	listen(sockfd, LISTEN_BACKLOG); //error handling?
	while (1) {
		// accept
		addr_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

		if (new_fd == -1) {
			fprintf(stderr, "accept error: %s\n", gai_strerror(new_fd));
			exit(1);
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		msg_len = strlen(msg);
		bytes_sent = send(new_fd, msg, msg_len, 0);

		if(bytes_sent == -1) {
			perror("Client read failed");
			exit(1);
		}
	}
}
