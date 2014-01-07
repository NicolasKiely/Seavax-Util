#ifndef PARSER_HEADER_FILE
#define PARSER_HEADER_FILE

#include "net.h"

#define PRS_SHUTDN 1
#define PRS_JOINPR 2
#define PRS_MAKEPR 3
#define PRS_REMVPR 4
#define PRS_CNTSRV 5
#define PRS_MSGSRV 6

void parse(struct netman *pNet, struct remote *pRem, char *pBuf, int len);

#endif
