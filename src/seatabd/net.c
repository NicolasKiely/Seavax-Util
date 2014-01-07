#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include "net.h"


int initListSock(const char port[]){
	int ret, fd;
	struct addrinfo aiInit;
	struct addrinfo *aiList = 0;
	
	/* Initialize addrinfo specs */
	memset(&aiInit, 0, sizeof(aiInit));
	aiInit.ai_family = AF_INET;
	aiInit.ai_socktype = SOCK_STREAM;
	aiInit.ai_flags = AI_PASSIVE;
	
	ret = getaddrinfo(NULL, port, &aiInit, &aiList);
	if (ret != 0 || aiList == 0){
		fprintf(stderr, "Error, cannot lookup addrinfo\n");
		return -1;
	}
	
	/* Create socket */
	fd = socket(aiList->ai_family, aiList->ai_socktype, aiList->ai_protocol);
	if (fd == -1){
		fprintf(stderr, "Error, could not create socket\n");
		freeaddrinfo(aiList);
		return -1;
	}
	
	/* Bind socket */
	ret = bind(fd, aiList->ai_addr, aiList->ai_addrlen);
	if (ret != 0){
		fprintf(stderr, "Error, could not bind socket\n");
		freeaddrinfo(aiList);
		close(fd);
		return -1;
	}
	
	int y = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y));
	
	/* Listen to port */
	ret = listen(fd, 8);
	if (ret != 0){
		fprintf(stderr, "Error, cannot listen to port\n");
		freeaddrinfo(aiList);
		close(fd);
		return -1;
	}
	
	freeaddrinfo(aiList);
	return fd;
}


void resetfds(struct netman *pNet){
	struct remote *pCur;
	struct profile *pPfl;
	
	FD_ZERO(&pNet->fdsr);
	
	FD_SET(pNet->lfd, &pNet->fdsr);
	pNet->fdsi = pNet->lfd+1;
	
	/* Run through list of remotes */
	for (pCur=pNet->pRem; pCur!=NULL; pCur=pCur->pNext){
		/* Add remote sock */
		FD_SET(pCur->sock, &pNet->fdsr);
		if (pNet->fdsi <= pCur->sock) pNet->fdsi = pCur->sock+1;
	}
	
	/* Run through list of profiles */
	for (pPfl=pNet->pPfl; pPfl != NULL; pPfl = pPfl->pNext){
		if (pPfl->srvSock < 0) continue;
	
		/* Add profile's connection */
		FD_SET(pPfl->srvSock, &pNet->fdsr);
		if (pNet->fdsi <= pPfl->srvSock) pNet->fdsi = pPfl->srvSock+1;
	}
}


void registerProfile(struct netman *pNet, struct profile *pPfl){
	if (pNet == NULL || pPfl == NULL) return;
	
	/* Push existing list after new profile, then add */
	pPfl->pNext = pNet->pPfl;
	pNet->pPfl = pPfl;
}


void addCon(struct netman *pNet, int newSock){
	struct remote *pNew = calloc(1, sizeof(struct remote));
	pNew->sock = newSock;
	pNew->str.defSize = BUCKET_SIZE;
	
	/* Disable blocking */
	fcntl(pNew->sock, F_SETFL, O_NONBLOCK);
	
	if (pNet->pRem == NULL){
		pNet->pRem = pNew;
	} else {
		pNew->pNext = pNet->pRem;
		pNet->pRem = pNew;
	}
	
	/* Create new profile. Name by first 4 bytes of memory address */
	pNew->pPfl = newProfile();
	pNew->pPfl->name = malloc(8);
	snprintf(pNew->pPfl->name, 7, "%p", (void *)((int) pNew % 0x10000));
	pNew->pPfl->name[7] = 0;
	
	registerProfile(pNet, pNew->pPfl);
}


void dropClosedRemotes(struct netman *pNet){
	struct remote *pOld = NULL;
	struct remote *pNxt = NULL;
	struct remote *pRmt = pNet->pRem;
	
	while (pRmt != NULL){
		if (pRmt->remFlag == 0){
			pOld = pRmt;
			pRmt = pRmt->pNext;
			continue;
		}
		
		/* Close socket and free read buffer */
		shutdownRemote(pRmt);
		clearString(&pRmt->str);
		
		//printf("DEBUG: Closed connection\n");
		
		/* Update previous remote */
		pNxt = pRmt->pNext;
		if (pOld == NULL){
			/* First client in list */
			pNet->pRem = pNxt;
			
		} else {
			pOld->pNext = pNxt;
		}
		
		logOutProfile(pRmt);
		free(pRmt);
		pRmt = pNxt;
	}
}


void shutdownRemote(struct remote *pRmt){
	if (pRmt == NULL) return;
	
	if (pRmt->sock >= 0){
		shutdown(pRmt->sock, SHUT_RDWR);
		close(pRmt->sock);
	}
	
	pRmt->sock = -1;
}


void shutdownRemoteR(struct remote *pRmt){
	if (pRmt == NULL) return;
	
	shutdownRemoteR(pRmt->pNext);
	shutdownRemote(pRmt);
}


void dropClosedProfiles(struct netman *pNet){
	struct profile *pOld = NULL;
	struct profile *pNxt = NULL;
	struct profile *pPfl = pNet->pPfl;
	
	while (pPfl != NULL){
		if (pPfl->remFlag == 0){
			pOld = pPfl;
			pPfl = pPfl->pNext;
			continue;
		}
		
		/* Close socket and free read buffer */
		//printf("DEBUG: Closed profile '%s'\n", pPfl->name);
		
		/* Update previous remote */
		pNxt = pPfl->pNext;
		if (pOld == NULL){
			/* First client in list */
			pNet->pPfl = pNxt;
			
		} else {
			pOld->pNext = pNxt;
		}
		
		freeProfile(&pPfl);
		pPfl = pNxt;
	}
}


void logOutProfile(struct remote *pRem){
	if (pRem == NULL) return;
	if (pRem->pPfl == NULL) return;
	
	/* Mark non-persistant profiles for removal */
	if (pRem->pPfl->isPrst == 0) pRem->pPfl->remFlag = -1;
	
	/* Remove from remote */
	pRem->pPfl = NULL;
}


int connectProfile(char *addrStr, char *port){
	int ret, fd;
	struct addrinfo aiInit;
	struct addrinfo *aiList = 0;
	
	if (addrStr == NULL || port == NULL){
		return -1;
	}
	
	/* Initialize addrinfo specs */
	memset(&aiInit, 0, sizeof(aiInit));
	aiInit.ai_family = AF_INET;
	aiInit.ai_socktype = SOCK_STREAM;
	aiInit.ai_flags = AI_PASSIVE;
	
	ret = getaddrinfo(addrStr, port, &aiInit, &aiList);
	if (ret != 0 || aiList == 0){
		return -1;
	}
	
	/* Create socket */
	fd = socket(aiList->ai_family, aiList->ai_socktype, aiList->ai_protocol);
	if (fd == -1){
		freeaddrinfo(aiList);
		return -1;
	}
	
	/* Attempt connection */
	ret = connect(fd, aiList->ai_addr, aiList->ai_addrlen);
	if (ret == -1){
		close(fd);
		freeaddrinfo(aiList);
		return -1;
	}
	
	/* Don't block on server socket*/
	fcntl(fd, F_SETFL, O_NONBLOCK);
	
	freeaddrinfo(aiList);
	return fd;
}


void sendMsgSrv(struct profile *pPfl, char *msg, int msgLen){
	int bytesSent, sent, fullLen, echoLen;
	char *fullMsg;
	
	if (pPfl == NULL || msg == NULL) return;
	if (pPfl->srvSock < 0) return;
	
	/* Calculate full message length */
	echoLen = strlen(pPfl->name);
	fullLen = msgLen + echoLen + 2;
	
	/* Create full message */
	fullMsg = malloc(fullLen);
	memcpy(fullMsg, pPfl->name, echoLen);
	fullMsg[echoLen] = '|';
	memcpy(fullMsg+echoLen+1, msg, msgLen);
	fullMsg[fullLen-1] = ';';
	
	/* Send */
	sendMsg(pPfl->srvSock, fullMsg, fullLen);
	
	free(fullMsg);
}


void sendMsg(int sockFD, char *msg, int msgLen){
	int bytesSent, sent, loop;

	if (sockFD < 0 || msg == NULL) return;
	
	loop = 0;
	bytesSent = 0;
	while (bytesSent < msgLen) {
		sent = send(sockFD, msg+bytesSent, msgLen-bytesSent, 0);
		
		if (sent < 0) return;
		
		bytesSent += sent;
		
		if (loop++ >= 100) break;
	}
}


void sendNullMsg(int sockFD){
	if (sockFD < 0) return;
	
	sendMsg(sockFD, "\n\n\n", 3);
}


void debNetman(struct netman *pNet){
	int rmtC, pflC;
	struct profile *pPfl;
	struct remote *pRmt;
	
	rmtC = 0;
	pflC = 0;
	
	for (pRmt = pNet->pRem; pRmt != NULL; pRmt = pRmt->pNext)
		if (pRmt->sock > 0) rmtC++;
	
	for (pPfl = pNet->pPfl; pPfl != NULL; pPfl = pPfl->pNext)
		if (pPfl->srvSock > 0) pflC++;
		
	printf("#net#:fdsi=%d rmt=%d pfl=%d\n", pNet->fdsi, rmtC, pflC);
}
