#include <stdio.h>
#include <stdlib.h>

namespace primihub {
int checkInstructionSupport(const char *name) {
	char cmdline[1024];
	char cmdmsg[1024];

	snprintf(cmdline, 1024, "cat /proc/cpuinfo | grep %s | wc -l", name);
	FILE *fp = popen(cmdline, "r");
	fgets(cmdmsg, 1024, fp);
    
	int line_num = atoi(cmdmsg);
	if (line_num == 0) 
		return -1;
	else
		return 0;
};
}
