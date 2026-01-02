#include "app.h"
#include "faceid.h"
#include <stdio.h>
#include <string.h>

static void usage(void) {
  printf("Usage:\n");
  printf("  faceid enroll\n");
  printf("  faceid lock\n");
  printf("  (press Q to quit)\n");
}

int app_run(int argc, char** argv, const AppConfig* cfg) {
  if (argc < 2) { usage(); return 0; }

  if (strcmp(argv[1], "enroll") == 0) {
    return faceid_enroll(cfg);
  }
  if (strcmp(argv[1], "lock") == 0) {
    return faceid_lock(cfg);
  }

  usage();
  return 0;
}
