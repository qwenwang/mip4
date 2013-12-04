#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "global.h"
#include "console.h"

#define KEY_BUFFER_SIZE 16
typedef struct {
	uint buff_size;
	int  key_ptr;
	char *buff_base;
} Key_buffer;

void 	init_key_buffer();
// 这个是类似硬件检查
int 	check_input_available();
// 从键盘中获得字符，并写到键盘缓冲区中
void 	write_to_key_buffer(char c);
char 	read_from_key_buffer();
#endif
