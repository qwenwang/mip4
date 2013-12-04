#include "utils.h"
void *
xmalloc (int size)
{
  void *x = (void *) malloc (size);

  if (x == 0)
    fatal_error ("Out of memory at request for %d bytes.\n");
  return x;
}


/* Allocate a zero'ed block of storage. */

void *
zmalloc (int size)
{
  void *z = (void *) malloc (size);

  if (z == 0)
    fatal_error ("Out of memory at request for %d bytes.\n");

  memset (z, 0, size);
  return z;
}

void
log_file(char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    vfprintf (log_fp, fmt, args);
    fflush(log_fp);
    va_end (args);
    fprintf(log_fp, "\n");
}


/* Print the error message then exit. */

void
fatal_error (char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fmt = va_arg (args, char *);

  vfprintf (stderr, fmt, args);
  exit -1;
}


/* Print an error message and return to top level. */

void
run_error (char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  // console_to_spim ();
  vfprintf (stderr, fmt, args);
  va_end (args);
  // longjmp (spim_top_level_env, 1);
}
