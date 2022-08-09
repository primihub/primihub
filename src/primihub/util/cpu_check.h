#include <stdio.h>
#include <stdlib.h>

namespace primihub {
int checkInstructionSupport(const char *name) {
  char cmdline[1024];
  char cmdmsg[1024*100];
  char fullname[1024];

  snprintf(fullname, 1024, "%s ", name);
  snprintf(cmdline, 1024, "cat /proc/cpuinfo | grep \"%s\" | wc -l", fullname);
  FILE *fp = popen(cmdline, "r");
  fgets(cmdmsg, 1024*100, fp);

  int line_num = atoi(cmdmsg);
  if (line_num == 0)
    return -1;
  else
    return 0;
};

void PrintCPUInfo(void) {
  char cmd[1024];
  snprintf(cmd, 1024, "cat /proc/cpuinfo");
  system(cmd);
}
} // namespace primihub
