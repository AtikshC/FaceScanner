#include "app.h"
#include "config.h"

int main(int argc, char** argv) {
  AppConfig cfg = config_default();
  return app_run(argc, argv, &cfg) ? 0 : 1;
}
