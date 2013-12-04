#ifndef CONSOLE_H
#define CONSOLE_H

#include "global.h"

typedef struct {
	int i;
	FILE *f;
} Port;

void init_console();
void write_direct_to_console (char *fmt, ...);
void echo_off();
void not_carriage();
void clean_screen();

extern Port console_in, console_out;

#endif