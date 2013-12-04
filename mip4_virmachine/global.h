#ifndef GLOBAL_H
#define GLOBAL_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdarg.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

typedef unsigned int inst32;
typedef int inst_imm;
typedef unsigned char inst_reg;
typedef unsigned char inst_opcode;
typedef int reg_word;
typedef int mem_word;
typedef int mem_addr;
typedef unsigned int u_reg_word;
typedef unsigned int addr_word;

#define K 				1024
#define BYTES_PER_WORD  4
#define BYTE    		1
#define H_WORD  		2
#define WORD    		4

typedef enum {
	B_RED = 0x10, B_GREEN = 0x20, B_LIGHT_RED = 0x30,
	B_BLUE = 0x40, B_PURPLE = 0x50, B_LIGHT_BLUE = 0x60,
	B_WHITE = 0x70, B_BLACK = 0x0,

	F_RED = 0x1, F_GREEN = 0x2, F_LIGHT_RED = 0x3,
	F_BLUE = 0x4, F_PUROLE = 0x5, F_LIGHT_BLUE = 0x6,
	F_BLACK = 0x0, F_WHITE = 0x7
} COLOR;

#define MAX_STEPS 65535
#define MEM_SIZE (1024 * (K))
#define ONE_SECTOR			512
#define ONE_CLUSTER 		((1) * (ONE_SECTOR))
#endif