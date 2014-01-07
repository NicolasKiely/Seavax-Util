#include <stdlib.h>
#include <string.h>

#include "bucket.h"


struct bucket *newBucket(int size){
	struct bucket *pBkt;
	int i;
	
	/* Check size */
	if (size <= 0) return NULL;
	
	/* Create */
	pBkt = malloc(sizeof(struct bucket));
	if (pBkt == NULL) return NULL;
	
	/* Initialize */
	pBkt->pBuf = malloc(1+size);
	if (pBkt->pBuf == NULL){
		free(pBkt);
		return NULL;
	}
	pBkt->size = size;
	pBkt->pNext = NULL;
	
	/* Clear buffer */
	for (i = 0; i <= pBkt->size; i++) pBkt->pBuf[i] = 0;
	
	return pBkt;
}


int isBucketNonEmpty(struct bucket *pBkt){
	if (pBkt == NULL) return 0;
	if (pBkt->pBuf == NULL) return 0;
	return -1;
}


int charInBucket(struct bucket *pBkt, char c){
	int i;
	
	if (pBkt == NULL) return -1;
	if (pBkt->pBuf == NULL) return -1;
	
	for (i = 0; i < pBkt->size; i++)
		if (pBkt->pBuf[i] == c) return i;
	
	return -1;
}


void rebaseBucket(struct bucket *pBkt, char *pc){
	char *pBuf;
	int len;
	
	if (pBkt == NULL) return;
	
	if (pc == NULL){
		/* Rebasing to nothing */
		clearBucket(pBkt);
		return;
	}
	
	len = strlen(pc);
	if (len <= 0){
		clearBucket(pBkt);
		return;
	}
	
	/* Start new buffer */
	pBuf = malloc(len + 1);
	if (pBuf == NULL) {
		/* Failed to allocate memory */
		clearBucket(pBkt);
		return;
	}
	
	/* Copy over buffer */
	pBuf[len] = 0;
	memcpy(pBuf, pc, len);
	
	/* Free original buffer and update */
	clearBucket(pBkt);
	pBkt->pBuf = pBuf;
	pBkt->size = len;
}


void clearBucket(struct bucket *pBkt){
	if (pBkt == NULL) return;
	
	if (pBkt->pBuf != NULL) free(pBkt->pBuf);
	pBkt->pBuf = NULL;
	pBkt->size = 0;
	pBkt->pNext = NULL;
}


void clearBucketR(struct bucket *pBkt){
	if (pBkt == NULL) return;
	
	freeBucketR(&(pBkt->pNext));
	clearBucket(pBkt);
}


void freeBucket(struct bucket **ppBkt){
	if (ppBkt == NULL) return;
	if (*ppBkt == NULL) return;
	
	clearBucket(*ppBkt);
	free(*ppBkt);
	*ppBkt = NULL;
}


void freeBucketR(struct bucket **ppBkt){
	if (ppBkt == NULL) return;
	if (*ppBkt == NULL) return;
	
	/* Go down list */
	freeBucketR( &((*ppBkt)->pNext) );
	
	/* At this point, pnext is freed and null. So free this bucket */
	freeBucket(ppBkt);
}
