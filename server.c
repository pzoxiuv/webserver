#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define READ_BUF_SIZE	1024

#define FILENAME	0
#define STRING		1

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr);
	else
		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

unsigned short get_in_port(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return ((struct sockaddr_in *)sa)->sin_port;
	else
		return ((struct sockaddr_in6 *)sa)->sin6_port;
}

void sigchild_handler(int unuzed)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char **argv)
{
	int yes = 1;
	int sockfd, newfd;
	int ret, bytes_sent, body_len, body_type;
	FILE *body_file;
	struct addrinfo hints, *server_info, *curr_ai;
	struct sockaddr_storage client_info;
	struct sigaction sa;
	socklen_t sin_size;
	char client_addr_name[INET6_ADDRSTRLEN];
	char read_buffer[READ_BUF_SIZE];
	char header_str[READ_BUF_SIZE];
	char *body_str;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s serverfile\n", argv[0]);
		exit(1);
	}

	lua_State *L = lua_open();
	luaL_openlibs(L);
	if (luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0))
		fprintf(stderr, "Error loading %s", argv[1]);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;

	lua_getglobal(L, "server");
	lua_pushstring(L, "port");
	lua_gettable(L, -2);
	if ((ret = getaddrinfo("0.0.0.0", lua_tostring(L, -1), &hints, &server_info)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(1);
	}
	lua_pop(L, 1);

	if (server_info == NULL) {
		fprintf(stderr, "server info null\n");
		exit(1);
	}

	for (curr_ai = server_info; curr_ai != NULL; curr_ai = curr_ai->ai_next) {
		/* Try opening a socket for this ai */
		if ((sockfd = socket(curr_ai->ai_family, curr_ai->ai_socktype, curr_ai->ai_protocol)) == -1) {
			perror("socket");
			continue;	// keep looping to try the next ai
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, curr_ai->ai_addr, curr_ai->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;	// if we get down here, everything above worked so no need to continue looping through curr_ai
	}

	freeaddrinfo(server_info);

	if (listen(sockfd, 20) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1) {
		sin_size = sizeof(client_info);
		newfd = accept(sockfd, (struct sockaddr *)&client_info, &sin_size);
		if (newfd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(client_info.ss_family, get_in_addr((struct sockaddr *)&client_info),
				  client_addr_name, sizeof(client_addr_name));
		printf("connection from %s:%hu\n", client_addr_name, ntohs(get_in_port((struct sockaddr *)&client_info)));

		if (!fork()) {	// this is the child
			close(sockfd);

			while (recv(newfd, read_buffer, sizeof(read_buffer), MSG_DONTWAIT) > 0);

			lua_getglobal(L, "server");
			lua_pushstring(L, "handleReq");
			lua_gettable(L, -2);
			lua_pushstring(L, read_buffer);
			if (lua_pcall(L, 1, 3, 0) != 0)
				fprintf(stderr, "%s\n", lua_tostring(L, -1));

			if ((int)lua_tonumber(L, -1) == STRING) {
				body_len = strlen(lua_tostring(L, -2));
				body_str = malloc(body_len);
				strncpy(body_str, lua_tostring(L, -2), body_len);
				body_type = STRING;
			}
			else if ((int)lua_tonumber(L, -1) == FILENAME) {
				body_file = fopen(lua_tostring(L, -2), "rb");
				fseek(body_file, 0, SEEK_END);
				body_len = ftell(body_file);
				fseek(body_file, 0, SEEK_SET);
				body_str = mmap(NULL, body_len, PROT_READ, MAP_PRIVATE, fileno(body_file), 0);
				body_type = FILENAME;
			}
			lua_pop(L, 2);

			strncpy(header_str, lua_tostring(L, -1), sizeof(header_str));
			lua_pop(L, 1);

			if ((bytes_sent = send(newfd, header_str, strlen(header_str), 0)) == -1)
				perror("send");
			if ((bytes_sent = send(newfd, body_str, body_len, 0)) == -1)
				perror("send");

			if (body_type == FILENAME) {
				munmap(body_str, body_len);
				fclose(body_file);
			} else if (body_type == STRING) {
				free(body_str);
			}

			shutdown(newfd, SHUT_RDWR);
			close(newfd);
			exit(0);
		}
		close(newfd);
	}

	lua_close(L);
}
