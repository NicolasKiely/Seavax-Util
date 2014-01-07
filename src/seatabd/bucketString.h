#ifndef BUCKETSTRING_HEADER_FILE
#define BUCKETSTRING_HEADER_FILE

#include "bucket.h"


/* Resizeable String */
struct bucketString{
	struct bucket *pBkt;  /* List of buckets */
	struct bucket *pLast; /* Last bucket */
	int defSize;          /* Default size of buckets */
};


/* Initializes and returns new string */
struct bucketString *newString(int defSize);


/* Returns -1 if no allocated buffers in string, or
 *  0 if one found.
 * Safe to call on NULL pointer
 */
int isStringNonEmpty(struct bucketString *pStr);


/* Compresses all string buckets to one bucket
 * Safe to call on NULL pointer
 */
void refactorString(struct bucketString *pStr);


/* Counts string's length
 * Safe to call on NULL pointer
 */
int getStringLen(struct bucketString *pStr);


/* Appends a bucket to a string
 * Safe to call on NULL pointer
 */
void appendBucket(struct bucketString *pStr, struct bucket *pBkt);


/* Creates and appends a new bucket to a string, then returns
 *  Safe to call on NULL pointer
 */
struct bucket *createAndAppendBucket(struct bucketString *pStr);


/* Returns first bucket buffer
 * Safe to call on NULL or empty string
 */
char *getStringBuffer(struct bucketString *pStr);


/* Clears internal string memory, frees buckets
 * Safe to call on NULL pointer
 */
void clearString(struct bucketString *pStr);


/* Frees String  struct 
 * Safe to call on NULL pointer
 */
void freeString(struct bucketString **ppStr);


#endif
