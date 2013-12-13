#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>


#define SEAVAX_PORT "6282"
#define ECHO_STR    "seatab|"
#define ECHO_LEN    strlen(ECHO_STR)
#define MAX_LOOP    100
#define HEAD_STATE  0
#define COL_STATE   1
#define REC_STATE   2
#define BUF_SIZE    1024


/* Attempts connection based on passed host string. Returns -1 on failure */
int connectToServer(char ipStr[]);

/* Sends a message to server. Handles formatting */
void sendMsg(int sockFD, char rawMsg[]);

/* Prints out socket results to stdout */
void printResults(int sockFD);


/*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
int main(int argc, char *argv[]){
	int sockFD;
	
	if (argc != 3) {
		fprintf(stderr, "Invalid arg count, usage: <ip address> <message>\n");
		return 1;
	}
	
	/* Connect */
	sockFD = connectToServer(argv[1]);
	if (sockFD < 0){
		fprintf(stderr,"Socket not established, exiting...\n");
		return 1;
	}
	
	/* Send message to server */
	sendMsg(sockFD, argv[2]);

	/* Read server response */
	printResults(sockFD);

	/* Shut down */
	close(sockFD);
	return 0;
}
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/


int connectToServer(char ipStr[]){
	int fd;
	int ret;
	struct addrinfo aiInit;
	struct addrinfo *aiList = 0;
	
	/* Initialize ai specs */
	memset(&aiInit, 0, sizeof(aiInit));
	aiInit.ai_family = AF_INET; /* Todo: Support both IPv6 too*/
	aiInit.ai_socktype = SOCK_STREAM;
	/*aiInit.ai_flags = AI_PASSIVE;*/
	
	/* Build list of address infos */
	ret = getaddrinfo(ipStr, SEAVAX_PORT, &aiInit, &aiList);
	if (ret != 0 || aiList == 0){
		fprintf(stderr, "Error, cannot lookup addrinfo\n");
		fprintf(stderr, "IP: %s, \terror: %d\n", ipStr, ret);
		perror("getaddrinfo: ");
		return -1;
	}
	
	/* For now, use first node */
	fd = socket(aiList->ai_family, aiList->ai_socktype, aiList->ai_protocol);
	if (fd == -1){
		fprintf(stderr, "Error, could not create socket\n");
		freeaddrinfo(aiList);
		return -1;
	}
	
	/* Attempt connection */
	ret = connect(fd, aiList->ai_addr, aiList->ai_addrlen);
	if (ret == -1){
		close(fd);
		fprintf(stderr, "Error, could not connect over socket\n");
		freeaddrinfo(aiList);
		return -1;
	}
	
	freeaddrinfo(aiList);
	return fd;
}


void sendMsg(int sockFD, char rawMsg[]){
	int rawLen, fullLen, bytesSent, loop;
	char *fullMsg;
	
	/* Calculate string lengths */
	rawLen = strlen(rawMsg);
	fullLen = rawLen + ECHO_LEN + 1;
	
	/* Set up new buffer */
	fullMsg = malloc(fullLen);
	
	strncpy(fullMsg, ECHO_STR, ECHO_LEN);
	strncpy(fullMsg+ECHO_LEN, rawMsg, rawLen);
	fullMsg[fullLen-1] = ';';
	
	/* Send buffer */
	bytesSent = 0;
	loop = 0;
	while (bytesSent < fullLen){
		int sent;
		
		sent = send(sockFD, fullMsg+bytesSent, fullLen-bytesSent, 0);
		if (sent > 0){
			bytesSent += sent;
		} else {
			fprintf(stderr, "Error thrown in sending message\n");
			break;
		}
		
		if (loop++ >= MAX_LOOP){
			fprintf(stderr, "Error, could not finish sending message\n");
			break;
		}
	}
	
	/* Free memory */
	free(fullMsg);
}


void printResults(int sockFD){
	int state, bytesRead, columns, breakLoop;
	int row, col;
	int loop=0;
	char *buf;
	
	/* Initialize buffer */
	buf = malloc(BUF_SIZE);
	
	state = HEAD_STATE;
	bytesRead = 0;
	columns = 1;
	col = 0;
	
	/* Read from socket*/
	breakLoop = 0;
	while (breakLoop == 0){
		int i;
		loop++;
	
		/* Reset buffer */
		memset(buf, 0, BUF_SIZE);
		
		bytesRead = recv(sockFD, buf, BUF_SIZE, 0);
		if (bytesRead <= 0){
			breakLoop = -1;
			break;
		}
		
		/* Processes buffer */
		for (i = 0; i < bytesRead; i++){
			char c = buf[i];
			
			if (state == HEAD_STATE){
				/* Reading header */
				if (c == '\n' || c == '\r'){
					state = COL_STATE;
					putc('\n', stdout);
					
				} else {
					putc(c, stdout);
				}
				
			} else if (state == COL_STATE) {
				/* Reading column */
				if (c == '\n' || c == '\r'){
					state = REC_STATE;
					putc('\n', stdout);
					/* columns++; */
					
				} else if (c == '\t'){
					columns++;
					putc(c, stdout);
					
				} else {
					putc(c, stdout);
				}
				
			} else {
				/* Reading records */
				if (c == '\n' || c == '\r'){
					breakLoop = -1;
					break;
					
				} else if (c == '\t'){
					col++;
					if (col >= columns){
						putc('\n', stdout);
						col = 0;
						
					} else {
						putc('\t', stdout);
					}
					
				} else {
					putc(c, stdout);
				}
			}
		}
		
		if (loop++ >= MAX_LOOP){
			fprintf(stderr, "Error, could not finish reading message\n");
			break;
		}
	}
	
	putc('\n', stdout);
	free(buf);
}
