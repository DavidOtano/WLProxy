#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Loopback.h"

#pragma warning (disable:4996)

int GetLoopback(void)
{
	int nRet = 127;
	char buf[1024], *p;
	FILE* fp = fopen("C:\\Windows\\System32\\drivers\\etc\\networks", "r");

	if(NULL != fp) {
		while(fgets(buf, 1024, fp)) {
			/* skip comments */
			if(*buf == '#')
				continue;

			/* ah, yes, just what we're lookin' for */
			if(strncmp(buf, "loopback", 8) == 0) {
				/* prepare the handy dandy iterator */
				p = buf + 8;

				/* skip those pesky whitespaces */
				for(; (*p == ' ' || *p == '\t'); ++p) ;

				/* the prize. */
				nRet = atoi(p);

				/* and we're out! */
				break;
			}
		}
		fclose(fp);
	}

	return nRet;
}
