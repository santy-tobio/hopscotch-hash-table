#include "testing.h"
#include <stdio.h>
#include <stdlib.h>

bool real_test_assert(const char *message, bool test_result, const char *file,
                      int line) {
  bool silent_defined = (getenv("TESTING_SILENT") != NULL);

  if (test_result && !silent_defined) {
    printf("%s:%d: OK - %s\n", file, line, message);
  } else if (!test_result) {
    printf("%s:%d: FAILED - %s\n", file, line, message);
  }
  fflush(stdout);
  return test_result;
}
