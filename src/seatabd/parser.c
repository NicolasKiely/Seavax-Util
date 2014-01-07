#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"


void parse(struct netman *pNet, struct remote *pRem, char *pBuf, int len){
	struct profile *pPfl;
	char *pArg;
	int argLen, ret;
	
	//printf("DEBUG msg: '%s'\n", pBuf);

	if (len < 6) {
		fprintf(stderr, "Message too short!\n");
		return;
		
	} else if (len == 6) {
		/* No argument passed */
		pArg = NULL;
		argLen = 0;
		
	} else {
		/* Some argument passed */
		pArg = pBuf+6;
		argLen = len - 6;
	}
	
	/* Lookup parse case */
	ret = 0;
	if (strncmp(pBuf, "shutDn", 6)==0){
		ret = PRS_SHUTDN;
		
	} else if (strncmp(pBuf, "joinPr", 6)==0){
		ret = PRS_JOINPR;
		
	} else if (strncmp(pBuf, "makePr", 6)==0){
		ret = PRS_MAKEPR;
		
	} else if (strncmp(pBuf, "remvPr", 6)==0){
		ret = PRS_REMVPR;
		
	} else if (strncmp(pBuf, "cntSrv", 6)==0){
		ret = PRS_CNTSRV;
		
	} else if (strncmp(pBuf, "msgSrv", 6)==0){
		ret = PRS_MSGSRV;
	}
	
	/* Handle case */
	if (ret == PRS_SHUTDN){
		/* Shut down daemon */
		pNet->sd = -1;
		
	} else if (ret == PRS_JOINPR || ret == PRS_MAKEPR) {
		/* Log out of old profile */
		logOutProfile(pRem);
	
		/* Look up profile */
		pPfl = rLookupProfile(pNet->pPfl, pArg);
		if (ret == PRS_MAKEPR) {
			/* Reset */
			resetProfile(pPfl);
		}
		
		if (pPfl == NULL) {
			/* No match found, create new */
			pPfl = newProfile();
			
			/* These profiles are persistent */
			pPfl->isPrst = -1;
			
			/* Set up name */
			pPfl->name = malloc(argLen + 1);
			memcpy(pPfl->name, pArg, argLen);
			pPfl->name[argLen] = 0;
			
			/* Register */
			registerProfile(pNet, pPfl);
		}
		
		/* Log into new profile */
		pRem->pPfl = pPfl;
		
	} else if (ret == PRS_REMVPR) {
		/* Remove profile */
		pPfl = rLookupProfile(pNet->pPfl, pRem->pPfl->name);
		pRem->pPfl = NULL;
		
		if (pPfl != NULL) pPfl->remFlag = -1;
		
	} else if (ret == PRS_CNTSRV) {
		/* Attempt to connect profile to server */
		if (pRem->pPfl == NULL){
			pRem->remFlag = -1;
			return;
		}
		
		/* Clean up any old connection on profile */
		resetProfile(pRem->pPfl);
		
		pRem->pPfl->srvSock = connectProfile(pArg, SEAVAX_PORT);
		
		if (pRem->pPfl->srvSock < 0) {
			pRem->remFlag = -1;
			return;
		}
		
	} else if (ret == PRS_MSGSRV) {
		/* Send message to server */
		if (pRem->pPfl == NULL){
			pRem->remFlag = -1;
			return;
		}
		
		sendMsgSrv(pRem->pPfl, pArg, argLen);
	}

}
