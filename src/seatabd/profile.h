#ifndef PROFILE_HEADER_FILE
#define PROFILE_HEADER_FILE

/* Profile information */
struct profile {
	char *name;  /* Profile name */
	int isPrst;  /* Persistence flag */
	int srvSock; /* Server socket */
	
	int remFlag; /* Removal flag */
	
	
	struct profile *pNext;
};


/* Initializes and returns new profile */
struct profile *newProfile();


/* Recursively looks up a profile by name, returns NULL if not found
 * Safe to call on NULL pointers
 */
struct profile *rLookupProfile(struct profile *pPfl, char *name);


/* Resets profile's connections
 * Safe to call on NULL pointer
 */
void resetProfile(struct profile *pPfl);


/* Non-recursively frees internal profile memory
 * Safe to call on NULL pointer
 */
void clearProfile(struct profile *pPfl);


/* Recursively frees internal profile memory
 * Safe to call on NULL pointer
 */
void clearProfileR(struct profile *pPfl);


/* Non-recursively frees profile structure
 * Safe to call on NULL pointer
 */
void freeProfile(struct profile **ppPfl);


/* Recursively frees profile structure
 * Safe to call on NULL pointer
 */
void freeProfileR(struct profile **ppPfl);

#endif
