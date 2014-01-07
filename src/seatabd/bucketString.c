#include <stdlib.h>
#include <string.h>

#include "bucketString.h"


struct bucketString *newString(int defSize){
	struct bucketString *pStr;

	/* Check size */
	if (defSize <= 0) return NULL;

	/* Allocate memory */
	pStr = malloc(sizeof(struct bucketString));
	if (pStr == NULL) return NULL;
	
	/* Initialize */
	pStr->defSize = defSize;
	pStr->pBkt = NULL;
	pStr->pLast = NULL;
	
	return pStr;
}


int isStringNonEmpty(struct bucketString *pStr){
	struct bucket *pBkt;
	
	if (pStr == NULL) return 0;
	for (pBkt=pStr->pBkt; pBkt != NULL; pBkt=pBkt->pNext){
		if (isBucketNonEmpty(pBkt) != 0) return -1;
	}
	
	return 0;
}


void refactorString(struct bucketString *pStr){
	struct bucket *pResult;
	struct bucket *pBkt;
	int i, j;
	
	/* Exit if string is empty, or only one bucket exists */
	if (pStr == NULL) return;
	if (pStr->pBkt == NULL){
		pStr->pLast = NULL;
		return;
	}
	if (pStr->pBkt->pNext == NULL){
		if (isBucketNonEmpty(pStr->pBkt)==0){
			/* Dump bucket */
			clearString(pStr);
			return;
			
		} else {
			/* Already have only one valid bucket */
			pStr->pLast = pStr->pBkt;
			return;
		}
	}
	
	/* Attempt to create new bucket */
	pResult = newBucket(getStringLen(pStr));
	
	if (pResult == NULL){
		/* Failed to create bucket. Clear string (probably empty) */
		clearString(pStr);
		return;
	}
	
	/* Copy over data from existing buckets */
	i = 0;
	for (pBkt = pStr->pBkt; pBkt != NULL; pBkt = pBkt->pNext){
		/* Skip empty buckets */
		if (isBucketNonEmpty(pBkt) == 0) continue;
		
		/* Copy buffers. i accumulates continuously */
		for (j = 0; j < pBkt->size; i++, j++){
			pResult->pBuf[i] = pBkt->pBuf[j];
		}
	}
	
	/* Remove old buckets and reassign to new bucket */
	clearString(pStr);
	pStr->pBkt = pResult;
	pStr->pLast = pResult;
}


int getStringLen(struct bucketString *pStr){
	struct bucket *pBkt;
	int len;
	
	if (pStr == NULL) return;
	
	len = 0;
	for (pBkt = pStr->pBkt; pBkt != NULL; pBkt = pBkt->pNext){
		if (pBkt->pBuf == NULL) continue;
		if (pBkt->size <= 0) continue;
		len += pBkt->size;
	}
	
	return len;
}


struct bucket *createAndAppendBucket(struct bucketString *pStr){
	struct bucket *pBkt;
	
	if (pStr == NULL) return NULL;

	/* Initialize and append new bucket */
	pBkt = newBucket(pStr->defSize);
	appendBucket(pStr, pBkt);
	
	return pBkt;
}


void appendBucket(struct bucketString *pStr, struct bucket *pBkt){
	struct bucket *pLast;

	if (pStr == NULL || pBkt == NULL) return;
	
	/* Append to end */
	if (pStr->pBkt == NULL){
		pStr->pBkt = pBkt;
	} else {
		pStr->pLast->pNext = pBkt;
	}
	
	/* Update pointer to last bucket */
	for (pLast = pBkt; pLast->pNext != NULL; pLast = pLast->pNext){;}
	pStr->pLast = pLast;
}


char *getStringBuffer(struct bucketString *pStr){
	if (pStr == NULL) return NULL;
	if (pStr->pBkt == NULL) return NULL;
	return pStr->pBkt->pBuf;
}


void clearString(struct bucketString *pStr){
	if (pStr == NULL) return;
	
	/* Free all buckets in string */
	freeBucketR(&(pStr->pBkt));
	pStr->pLast = NULL;
}


void freeString(struct bucketString **ppStr){
	if (ppStr == NULL) return;
	if (*ppStr == NULL) return;
	
	clearString(*ppStr);
	free(*ppStr);
	
	*ppStr = NULL;
}
