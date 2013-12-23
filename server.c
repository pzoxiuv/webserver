#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define READ_BUF_SIZE	1024

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

int main(void)
{
	int sockfd, newfd;
	struct addrinfo hints, *server_info, *curr_ai;
	struct sockaddr_storage client_info;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	int ret, bytes_sent;
	char client_addr_name[INET6_ADDRSTRLEN];
	char response_str[1024];
	char read_buffer[READ_BUF_SIZE];

	lua_State *L = lua_open();
	luaL_openlibs(L);
	if (luaL_loadfile(L, "example.lua") || lua_pcall(L, 0, 0, 0))
		fprintf(stderr, "Error loading example.lua");


	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo("127.0.0.1", "8888", &hints, &server_info)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(1);
	}

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
			lua_pushstring(L, "parseReq");
			lua_gettable(L, -2);
			lua_pushstring(L, read_buffer);
			if (lua_pcall(L, 1, 1, 0) != 0)
				fprintf(stderr, "%s\n", lua_tostring(L, -1));

			strncpy(response_str, lua_tostring(L, -1), sizeof(response_str));
			lua_pop(L, 1);

			if ((bytes_sent = send(newfd, response_str, strlen(response_str), 0)) == -1)
				perror("send");

			shutdown(newfd, SHUT_RDWR);
			close(newfd);
			exit(0);
		}
		close(newfd);
	}

	lua_close(L);
}
