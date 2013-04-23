
#include "../Common/Common.h"

int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (argc != 2)
		exit(1);

	char buffer[BUFFER_SIZE];
	portno = atoi(argv[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	server = gethostbyname("localhost");
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char *)server->h_addr,
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");

	printf("Please enter the message: ");
	memset(buffer, 0, BUFFER_SIZE);
	gets(buffer);
	while (strcmp(buffer, "EXIT") && strcmp(buffer, "EXITALL")) {
		n = send(sockfd, buffer, strlen(buffer), 0);
		if (n < 0)
			error("ERROR writing to socket");
		memset(buffer, 0, BUFFER_SIZE);
		n = recv(sockfd, buffer, 255, 0);
		if (n < 0)
			error("ERROR reading from socket");
		printf("%s\nPlease enter the message: ", buffer);
		memset(buffer, 0, BUFFER_SIZE);
		gets(buffer);
	}
	send(sockfd, buffer, strlen(buffer), 0);
	close(sockfd);
	return 0;
}
