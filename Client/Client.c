#include "../Common/Common.h"
#include "../Common/Utilities/StringUtil.h"
char MY_PATH[BUFFER_SIZE] = "";
char sIP[BUFFER_SIZE];
char sPORT[BUFFER_SIZE];
int sockfd = -1;
fd_set file_descriptors;
int max_sd;
int init(char* serverIP, char* port) {

	// Allready connected
	if (sockfd > 0)
		return 1;

	int len, rc;
	struct sockaddr_in addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// Socket couldn't be created
	if (sockfd < 0) {
		error("Error: Socket cannot be established");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, serverIP, &addr.sin_addr);
	addr.sin_port = htons(atoi(port));

	// Tring to connect
	rc = connect(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));

	// If the attempt was unsuccessful
	if (rc < 0) {
		error("Error: Unable to connect");
		close(sockfd);
		sockfd = -1;
		return -1;
	}

	// Adds this new connectiong to file descriptors set
	FD_SET(sockfd, &file_descriptors);

	// Sets the maximum descriptor to this fd, STDIN is 0.
	max_sd = sockfd;

	return 0;
}

int do_connect(char* line) {
	int locIndex = 0;
	memset(sIP, 0, BUFFER_SIZE);
	locIndex = nextTokenDelimiter(line, sIP, 8, ':');

	memset(sPORT, 0, BUFFER_SIZE);
	nextToken(line, sPORT, locIndex);
	int rc = init(sIP, sPORT);
	return rc;
}

// Sends a line of char via sockfd
int do_send(char* line) {
	int len = send(sockfd, line, strlen(line) + 1, 0);
	if (len != strlen(line) + 1) {
		error("Error is send");
		return -1;
	}
	return 0;
}

// Sends a char* with it's size
int do_send_size(char* line, int size) {
	int len = send(sockfd, line, size, 0);
	if (len != size) {
		error("Error is send");
		return -1;
	}
	return 0;
}

// Receives a char* with maximum of BUFFER_SIZE via sockfd
int do_receive(char* output) {
	memset(output, 0, sizeof(char) * BUFFER_SIZE);
	return recv(sockfd, output, sizeof(char) * BUFFER_SIZE, 0);
}

// Runs commands, depending on them
int do_command(char* line) {
	int index = 0;
	char next[BUFFER_SIZE] = "";
	char send_buf[BUFFER_SIZE] = "";
	char recv_buf[BUFFER_SIZE] = "";
	memset(next, 0, BUFFER_SIZE);
	index = nextToken(line, next, index);
	int k = 100;
	if (strcmp(next, "quit") == 0)
		k = -1; // quit
	else if (strcmp(next, "connect") == 0) {
		k = do_connect(line);
		printf("You are connected!\n");
		k = 0;
	} else if (strcmp(next, "share") == 0) {
		if (sockfd < 0) {
			printf("You are not connected.\n");
			return 100;
		}
		index = nextToken(line, next, index);
		char fileName[BUFFER_SIZE];
		strcpy(fileName, MY_PATH);
		strcat(fileName, next);
		FILE* tmpFile = fopen(fileName, "r");
		if (tmpFile == NULL )
			error("This file doesn't exist!");
		else {
			fclose(tmpFile);
			strcpy(send_buf, "SH");
			strcat(send_buf, next);
			strcat(send_buf, " ");
			char tmpInt[BUFFER_SIZE] = "";
			struct stat info;
			stat(fileName, &info);
			convertIntToString(info.st_size, tmpInt);
			strcat(send_buf, tmpInt);
			fprintf(stderr, "MSG: %s\n", send_buf);
			do_send(send_buf);
			fprintf(stderr, "Waiting for receive!\n");
			do_receive(recv_buf);
			fprintf(stderr, "before if: %s\n", recv_buf);
			if (strcmp(recv_buf, "OK") != 0) {
				fprintf(stderr, "Err: %s\n", recv_buf);
				error("There is a problem with sharing file!");
			} else
				printf("Your file has been shared!\n");
			fprintf(stderr, "after if\n");
		}
		k = 2; // share
	} else if (strcmp(next, "get-files-list") == 0) {
		if (sockfd < 0) {
			printf("You are not connected.\n");
			return 100;
		}
		k = do_send("FL");
		if (k >= 0) {
			k = do_receive(recv_buf);
			if (k >= 0) {
				printf("----------shared files-----------\n");
				printf("%s\n", recv_buf);
			}
		}
		k = 3; // share
	} else if (strcmp(next, "get") == 0) {
		pid_t pid = fork();
		printf("New Born HAHAHAHAH %d\n", pid);
		if (pid == 0) {
			printf("I'm the child\n");
			index = nextToken(line, next, index);
			char fileName[BUFFER_SIZE];
			strcpy(fileName, MY_PATH);
			strcat(fileName, next);
			char* tmpArg[] = { "Getter", sIP, fileName, sPORT, (char*) 0 };
//			argv[0]= { sIP, fileName, NULL };
			if (execv("./Getter", tmpArg) != 0)
				error("There is a problem with getting file!");
			printf("I'm quiting now!\n");
			exit(0);
		}
		k = 4; // get
	} else if (strcmp(next, "dc") == 0) {
		k = do_send("EXIT");
		printf("You are disconnected now!\n");
	}
	return k;
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		error("Error: Use ./client directory");
		return -1;
	}
	strcpy(MY_PATH, argv[1]);

	int len, rc, status, IS_ALIVE = 1;
	int i;
	char buffer[BUFFER_SIZE];

	max_sd = STDIN;
	FD_ZERO(&file_descriptors);
	FD_SET(STDIN, &file_descriptors);
	fd_set working_set;

	do {

		memcpy(&working_set, &file_descriptors, sizeof(file_descriptors));

		rc = select(max_sd + 1, &working_set, NULL, NULL, NULL );

		if (rc < 0) {
			error("  select() failed");
			break;
		}

		if (rc == 0) {
			error("  select() timed out.  End program.");
			break;
		}

		for (i = 0; i <= max_sd && IS_ALIVE; ++i) {
			if (FD_ISSET(i, &working_set)) {
				if (i == STDIN) {

					memset(buffer, 0, BUFFER_SIZE);
					//readLine(buffer);
					gets(buffer);
					status = do_command(buffer);
					if (status == -1)
						IS_ALIVE = FALSE;
				} else {
					// Socket is hot, should now recv a message from server
					printf("I'm in select socket\n");
					do_receive(buffer);
					printf("Message%s\n", buffer);
					do_send("TOF12568");
					printf("I sent message\n");
				}
			}
		}
		printf("--> ");
	} while (IS_ALIVE);
	return 0;
}
