#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

void log(const char *const fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}
