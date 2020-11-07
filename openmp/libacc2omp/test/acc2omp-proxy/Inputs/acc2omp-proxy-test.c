// expected-no-diagnostics

#include "acc2omp-proxy.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void acc2omp_fatal(acc2omp_msg_t Msg, ...) {
  fprintf(stderr, "acc2omp-proxy-test: fatal error: ");
  va_list Args;
  va_start(Args, Msg);
  vfprintf(stderr, Msg.DefaultFmt, Args);
  va_end(Args);
  fprintf(stderr, "\n");
  exit(1);
}

void acc2omp_warn(acc2omp_msg_t Msg, ...) {
  fprintf(stderr, "acc2omp-proxy-test: warning: ");
  va_list Args;
  va_start(Args, Msg);
  vfprintf(stderr, Msg.DefaultFmt, Args);
  va_end(Args);
  fprintf(stderr, "\n");
}

void acc2omp_assert(int Cond, const char *Msg, const char *File, int Line) {
  fprintf(stderr, "acc2omp-proxy-test: assertion failed at %s:%d: %s\n", File,
          Line, Msg);
  exit(1);
}
