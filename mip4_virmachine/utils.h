#ifndef UTILS_H
#define UTILS_H
#include "global.h"

void *xmalloc (int size);
void *zmalloc (int size);
void log_file (char *fmt, ...);
/* Print the error message then exit. */
void fatal_error (char *fmt, ...);
/* Print an error message and return to top level. */
void run_error (char *fmt, ...);

extern FILE *log_fp;
#endif
