#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>


#include "bucketString.h"
#include "net.h"
#include "parser.h"


/* Socket reading code */
void checkRemoteLoop(struct netman *pNet);
void checkSeavaxLoop(struct netman *pNet);


/* Commands:
 *  makePr  - Creates a new profile, resets if exists
 *  joinPr  - Joins a profile, creates if doesn't exist
 *  remvPr  - Removes a profile
 *  cntSrv  - Connects profile to a server
 *  msgSrv  - Sends message to server
 *  shutDn  - Shuts down process
 */

/*vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
int main(int argc, char *argv[]){
	struct netman net;
	
	/* Start up */
	memset(&net, 0, sizeof(net));
	net.lfd = initListSock(LISTENING_PORT);
	if (net.lfd == -1){
		fprintf(stderr, "init error!\n");
		return 1;
	}
	
	/* Main loop */
	while (net.sd == 0){
		struct remote *pCur;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 1000*20;
	
		/* Check sockets */
		//printf("\nPolling sockets; ");
		//debNetman(&net);
		resetfds(&net);
		select(net.fdsi, &net.fdsr, NULL, NULL, NULL);
		//select(net.fdsi, &net.fdsr, NULL, NULL, &tv);
		
		/* Handle listener */
		if (FD_ISSET(net.lfd, &net.fdsr)){
			/* Read listening port */
			addCon(&net, accept(net.lfd, NULL, NULL));
			//printf("DEBUG: Adding new client\n");
		}
		
		/* Check remote sockets */
		//printf("Reading client sockets; ");
		//debNetman(&net);
		checkRemoteLoop(&net);
		
		//printf("Reading server sockets; ");
		//debNetman(&net);
		checkSeavaxLoop(&net);
		
		/* Clean up discarded structs */
		dropClosedRemotes(&net);
		dropClosedProfiles(&net);
	}
	
	/* Shutdown */
	shutdownRemoteR(net.pRem);
	freeProfileR(&(net.pPfl));
	if (net.lfd >= 0) close(net.lfd);
	return 0;
}
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/


void checkRemoteLoop(struct netman *pNet){
	struct remote *pCur;
	
	
	for (pCur=pNet->pRem; pCur!=NULL; pCur=pCur->pNext){
		char *stringp;
		char *stringSep;
			
		if (FD_ISSET(pCur->sock, &pNet->fdsr)==0) continue;
			
		/* Read into buffer */
		while(-1){
			int readLen;
			struct bucket *pBkt = newBucket(pCur->str.defSize);
			
			readLen = recv(pCur->sock, pBkt->pBuf, pBkt->size, 0);
			
			if (readLen <= 0){
				/* No data read, no use for bucket */
				freeBucket(&pBkt);
				if (readLen == 0) pCur->remFlag = -1;
				break;
			}
			
			/* Append bucket to string */
			pBkt->size = readLen;
			appendBucket(&(pCur->str), pBkt);
		}
		
		/* Compress buckets into one buffer */
		refactorString(&(pCur->str));
		
		while (-1){
			/* Get string's buffer */
			stringp = getStringBuffer(&(pCur->str));
			if (stringp != NULL){
				/* Separate string */
				stringSep = strsep(&stringp, "\n");
			
				if (stringp != NULL) {
					/* Match found. Parse and rebase */
					parse(pNet, pCur, stringSep, strlen(stringSep));
				
					/* Rebase bucket. Will free original buffer */
					rebaseBucket(pCur->str.pBkt, stringp);
					
				} else {
					break;
				}
				
			} else {
				break;
			}
		} /* while (-1) */
	} /* For pCur */
}


void checkSeavaxLoop(struct netman *pNet){
	struct profile *pCur;
	struct remote  *pRmt;
	struct remote  *pIbkt;
	
	for (pCur = pNet->pPfl; pCur != NULL; pCur = pCur->pNext){
		struct bucketString *pStr;
	
		if (pCur->srvSock < 0) continue;
		if (FD_ISSET(pCur->srvSock, &pNet->fdsr)==0) continue;
		
		pStr = newString(BUCKET_SIZE);
		
		/* Read data into buffer */
		while (-1){
			int readLen;
			struct bucket *pBkt = newBucket(pStr->defSize);
			
			readLen = recv(pCur->srvSock, pBkt->pBuf, pBkt->size, 0);
			
			if (readLen <= 0){
				/* No data read, no use for bucket */
				freeBucket(&pBkt);
				break;
			}
			
			/* Append bucket to string */
			pBkt->size = readLen;
			appendBucket(pStr, pBkt);
		}
		
		/* Compress buckets into one buffer */
		refactorString(pStr);
		if (isBucketNonEmpty(pStr->pBkt) == 0){
			/* Send empty message */
			//printf("#SRV#: #NULL#\n");
			sendNullMsg(pRmt->sock);
			
			freeString(&pStr);
			continue;
		}
		
		/* DEBUG */
		//printf("#SRV#: '%s'\n", pStr->pBkt->pBuf);
		
		/* Broadcast to linked remotes */
		for (pRmt = pNet->pRem; pRmt != NULL; pRmt = pRmt->pNext){
			if (pRmt->pPfl == pCur){
				sendMsg(pRmt->sock, pStr->pBkt->pBuf, pStr->pBkt->size);
				sendMsg(pRmt->sock, "\n", 1);
				//pRmt->remFlag = -1;
			}
		}
		
		freeString(&pStr);
		
	} /* For pCur*/
}
