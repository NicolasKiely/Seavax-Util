#ifndef NET_HEADER_FILE
#define NET_HEADER_FILE
/* Networking code */

#include <sys/select.h>

#include "bucketString.h"
#include "profile.h"

#define LISTENING_PORT "6281"
#define SEAVAX_PORT    "6282"


/* Client connection information */
struct remote{
	int remFlag; /* Remove struct flag */
	int sock;    /* Socket connection */
	
	struct profile *pPfl;    /* Profile for remote */
	struct bucketString str; /* Working recv buffer */
	//struct bucketString msg; /* Seavax message buffer */
	
	struct remote *pNext;    /* Next struct */
};


/* Program's network data */
struct netman {
	int lfd;     /* Listening socket */
	fd_set fdsr; /* fd read set */
	int fdsi;    /* fd set count */
	int sd;      /* Shut down */    
	
	struct remote  *pRem; /* remote list */
	struct profile *pPfl; /* profile list */
};


/* Returns initialized socket for listening on port, or -1 on error */
int initListSock(const char port[]);

/* Resets file descriptor list */
void resetfds(struct netman *pNet);

/* Adds a connection to network manager */
void addCon(struct netman *pNet, int newSock);


/* Registers profile with network manager
 * Safe to call on NULL pointers
 */
void registerProfile(struct netman *pNet, struct profile *pPfl);


/* Drops closed client connections */
void dropClosedRemotes(struct netman *pNet);


/* Non-recursively shuts down and closes remote socket
 * Safe to call on NULL pointers
 */
void shutdownRemote(struct remote *pRmt);


/* Recursively shuts down and closes remote socket
 * Safe to call on NULL pointers
 */
void shutdownRemoteR(struct remote *pRmt);


/* Drops closed profiles */
void dropClosedProfiles(struct netman *pNet);


/* Logs remote out of profile
 * Safe to call on NULL pointer
 */
void logOutProfile(struct remote *pRem);


/* Attempts to connect to a seavax server */
int connectSeavax(char *addrStr, char *port);


/* Sends a message to a server through a profile */
void sendMsgSrv(struct profile *pPfl, char *msg, int msgLen);


/* Sends a message over a socket */
void sendMsg(int sockFD, char *msg, int msgLen);


/* Sends null table message */
void sendNullMsg(int sockFD);

#endif
