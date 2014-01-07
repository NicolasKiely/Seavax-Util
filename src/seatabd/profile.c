#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "profile.h"

struct profile *newProfile(){
	struct profile *pPfl;
	
	pPfl = malloc(sizeof(struct profile));
	if (pPfl == NULL) return NULL;
	pPfl->name = NULL;
	pPfl->isPrst = 0;
	pPfl->pNext = NULL;
	pPfl->remFlag = 0;
	pPfl->srvSock = -1;
	
	return pPfl;
}


struct profile *rLookupProfile(struct profile *pPfl, char *name){
	if (pPfl == NULL) return NULL;
	
	if (pPfl->name == NULL || name == NULL){
		if (pPfl->name == NULL && name == NULL){
			/* Matched nulls */
			return pPfl;
			
		} else {
			/* Only one is null */
			return NULL;
		}
	} 
	
	if (strcmp(pPfl->name, name) == 0){
		/* Found a match */
		return pPfl;
		
	} else {
		/* Continue searching */
		return rLookupProfile(pPfl->pNext, name);
	}
}


void resetProfile(struct profile *pPfl){
	if (pPfl == NULL) return;
	
	if (pPfl->srvSock >= 0){
		shutdown(pPfl->srvSock, SHUT_RDWR);
		close(pPfl->srvSock);
	}
	pPfl->srvSock = -1;
}


void clearProfile(struct profile *pPfl){
	if (pPfl == NULL) return;
	
	if (pPfl->name != NULL) free(pPfl->name);
	pPfl->name = NULL;
	pPfl->pNext = NULL;
	resetProfile(pPfl);
}


void clearProfileR(struct profile *pPfl){
	if (pPfl == NULL) return;
	
	freeProfileR(&(pPfl->pNext));
	clearProfile(pPfl);
}


void freeProfile(struct profile **ppPfl){
	if (ppPfl == NULL) return;
	if (*ppPfl == NULL) return;
	
	clearProfile(*ppPfl);
	free(*ppPfl);
	*ppPfl = NULL;
}


void freeProfileR(struct profile **ppPfl){
	if (ppPfl == NULL) return;
	if (*ppPfl == NULL) return;
	
	freeProfileR( &((*ppPfl)->pNext) );
	freeProfile(ppPfl);
}
