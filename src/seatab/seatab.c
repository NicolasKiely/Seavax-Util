#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>


#define SEAVAX_PORT  "6282"
#define SEATABD_PORT "6281"
#define PREFIX_LEN   7
#define MAX_LOOP     100
#define HEAD_STATE   0
#define COL_STATE    1
#define REC_STATE    2
#define BUF_SIZE     1024


/* Program Flags. Ints are booleans testing for
    flag existence. Chars* are arguments
 */
struct flags{
	/* Controls stdout output */
	int b;           /* output table body    */
	int c;           /* output table columns */
	int h;           /* output table header  */
	
	/* Profile control (sent first) */
	int r;           /* remove profile       */
	int j; char *sj; /* join profile         */
	int p; char *sp; /* join fresh profile   */
	
	/* Server communication (sent second) */
	int i; char *si; /* seavax ip address    */
	int m; char *sm; /* message to send      */
	
	/* Daemon control (sent last) */
	int x;           /* shutdown seatabd     */
};


/* Attempts connection based on passed host string. Returns -1 on failure */
int connectToDaemon(char ipStr[]);

/* Sends a message to daemon. Appends with \n */
void sendRawMsg(int sockFD, char *preMsg, char *rawMsg);

/* Sends flag-specified messages to daemon */
void sendMsgs(int sockFD, struct flags *pf);

/* Prints out socket results to stdout */
void printResults(int sockFD, struct flags *pf);

/* Fills out flag structure from arguments */
void processFlags(struct flags *pf, int argc, char *argv[]);

/* Prints usage string to stdout */
void displayUsage();


/*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
int main(int argc, char *argv[]){
	int sockFD;
	struct flags f;
	
	/* Display usage */
	if (argc == 1){
		displayUsage();
		return 0;
	}
	
	/* Run flags */
	processFlags(&f, argc, argv);
	
	/* Connect to daemon. May allow remote connections in future */
	sockFD = connectToServer("127.0.0.1");
	if (sockFD < 0){
		fprintf(stderr,"Socket not established, exiting...\n");
		return 1;
	}
	
	/* Send information */
	sendMsgs(sockFD, &f);

	/* Read server response */
	if (f.m != 0 && (f.b!=0 || f.c!=0 || f.h!=0))
		printResults(sockFD, &f);

	/* Shut down */
	shutdown(sockFD, SHUT_RDWR);
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
	ret = getaddrinfo(ipStr, SEATABD_PORT, &aiInit, &aiList);
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


void sendMsgs(int sockFD, struct flags *pf){
	if (pf->j != 0) sendRawMsg(sockFD, "joinPr", pf->sj);
	if (pf->p != 0) sendRawMsg(sockFD, "makePr", pf->sp);
	if (pf->i != 0) sendRawMsg(sockFD, "cntSrv", pf->si);
	if (pf->m != 0) sendRawMsg(sockFD, "msgSrv", pf->sm);
	if (pf->r != 0) sendRawMsg(sockFD, "remvPr", NULL);
	if (pf->x != 0) sendRawMsg(sockFD, "shutDn", NULL);
}


void sendRawMsg(int sockFD, char *preMsg, char *rawMsg){
	int bytesSent, rawLen, preLen, sent, loop;
	char cap;
	
	cap = '\n';
	bytesSent = 0;
	loop = 0;
	preLen = (preMsg != NULL) ? strlen(preMsg) : 0;
	rawLen = (rawMsg != NULL) ? strlen(rawMsg) : 0;
	
	/* Send pre message */
	while (bytesSent < preLen){
		sent = send(sockFD, preMsg+bytesSent, preLen-bytesSent, 0);
		
		if (sent > 0){
			bytesSent += sent;
		} else if (send < 0){
			fprintf(stderr, "Error thrown in sending message\n");
			break;
		}
		
		if (loop++ > MAX_LOOP){
			fprintf(stderr, "Error, timed out in sending message\n");
		}
	}
	
	/* Send message */
	loop = 0;
	bytesSent = 0;
	
	while (bytesSent < rawLen){
		sent = send(sockFD, rawMsg+bytesSent, rawLen-bytesSent, 0);
		
		if (sent > 0){
			bytesSent += sent;
		} else if (send < 0){
			fprintf(stderr, "Error thrown in sending message\n");
			break;
		}
		
		if (loop++ > MAX_LOOP){
			fprintf(stderr, "Error, timed out in sending message\n");
		}
	}
	
	/* Send message cap */
	sent = 0;
	loop = 0;
	while (sent == 0){
		sent = send(sockFD, &cap, 1, 0);
		if (sent < 0){
			fprintf(stderr, "Error thrown in capping message\n");
		}
		if (loop++ > MAX_LOOP){
			fprintf(stderr, "Error, timed out in sending message cap\n");
		}
	}
}


void printResults(int sockFD, struct flags *pf){
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
					if (pf->h!=0) putc('\n', stdout);
					
				} else {
					if (pf->h!=0) putc(c, stdout);
				}
				
			} else if (state == COL_STATE) {
				/* Reading column */
				if (c == '\n' || c == '\r'){
					state = REC_STATE;
					if (pf->c!=0) putc('\n', stdout);
					/* columns++; */
					
				} else if (c == '\t'){
					columns++;
					if (pf->c!=0) putc(c, stdout);
					
				} else {
					if (pf->c!=0) putc(c, stdout);
				}
				
			} else {
				/* Reading records */
				if (c == '\n' || c == '\r'){
					breakLoop = -1;
					break;
					
				} else if (c == '\t'){
					col++;
					if (col >= columns){
						if (pf->b!=0) putc('\n', stdout);
						col = 0;
						
					} else {
						if (pf->b!=0) putc('\t', stdout);
					}
					
				} else {
					if (pf->b!=0) putc(c, stdout);
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


void processFlags(struct flags *pf, int argc, char *argv[]){
	int i;
	char **pfs;
	
	pfs = NULL;
	bzero(pf, sizeof(struct flags));
	
	for (i = 0; i < argc; i++){
		if (pfs != NULL){
			/* String argument */
			*pfs = argv[i];
			pfs = NULL;
			continue;
		}
		
		/* Flag */
		if (strcmp(argv[i], "-b") == 0){
			pf->b = -1;
		} else if (strcmp(argv[i], "-c") == 0){
			pf->c = -1;
		} else if (strcmp(argv[i], "-h") == 0){
			pf->h = -1;
		} else if (strcmp(argv[i], "-i") == 0){
			pf->i = -1;
			pfs = &pf->si;
		} else if (strcmp(argv[i], "-j") == 0){
			pf->j = -1;
			pfs = &pf->sj;
		} else if (strcmp(argv[i], "-m") == 0){
			pf->m = -1;
			pfs = &pf->sm;
		} else if (strcmp(argv[i], "-p") == 0){
			pf->p = -1;
			pfs = &pf->sp;
		} else if (strcmp(argv[i], "-r") == 0){
			pf->r = -1;
		} else if (strcmp(argv[i], "-x") == 0){
			pf->x = -1;
		}
	}
	
	/* Handle unfinished flags */
	if (pf->si == NULL) pf->i = 0;
	if (pf->sj == NULL) pf->j = 0;
	if (pf->sm == NULL) pf->m = 0;
	if (pf->sp == NULL) pf->p = 0;
}


void displayUsage(){
	printf("Usage:\n\t./seatab [flags]\n");
	
	printf("\nProfile Flags:\n");
	printf("\t-j <name>: Joins the named profile\n");
	printf("\t-p <name>: Joins the named profile and resets it\n");
	printf("\t-r       : Removes profile specified by -j\n");
	
	printf("\nServer Flags:\n");
	printf("\t-i <ip address>: Connects profile to server at the ip address\n");
	printf("\t-m <message>   : Sends message to profile's server connection\n");
	
	printf("\nDisplay Flags:\n");
	printf("\t-h: Displays table header of server's response\n");
	printf("\t-c: Displays table columns of server's response\n");
	printf("\t-b: Displays table body of server's response\n");
	
	printf("\nDaemon Flags:\n");
	printf("\t-x: Shut down daemon\n");
	printf("\n");
}
