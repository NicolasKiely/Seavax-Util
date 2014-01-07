#ifndef BUCKET_HEADER_FILE
#define BUCKET_HEADER_FILE


#define BUCKET_SIZE 128


/* Buffer in string. Buffer is allocated to 1+size
 * bytes, with last byte set to 0
 */
struct bucket{
	char *pBuf; /* Memory buffer */
	int size;   /* Buffer size */
	
	struct bucket *pNext;
};


/* Initializes and returns new bucket */
struct bucket *newBucket(int size);


/* Returns -1 if buffer is not null, 0 if null
 * Safe to call on NULL pointer
 */
int isBucketNonEmpty(struct bucket *pBkt);


/* Returns position of char in bucket, or -1 if not found
 * Safe to call on NULL pointer
 */
int charInBucket(struct bucket *pBkt, char c);


/* Rebuilds bucket buffer from updated pointer. Cleans
 * original buffer. Safe to call on NULL pointers
 */
void rebaseBucket(struct bucket *pBkt, char *pc);


/* Non-recursively frees internal struct memory
 * Safe to call on NULL pointer
 */
void clearBucket(struct bucket *pBkt);


/* Cleans bucket and recursively frees any sub-buckets
 * Safe to call on NULL pointer
 */
void clearBucketR(struct bucket *pBkt);


/* Non-recursively frees struct
 * Safe to call on NULL pointer
 */
void freeBucket(struct bucket **ppBkt);


/* Recursively frees struct
 * Safe to call on NULL pointer
 */
void freeBucketR(struct bucket **ppBkt);

#endif
