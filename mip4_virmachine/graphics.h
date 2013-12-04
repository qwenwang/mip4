#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "global.h"

#define DISPLAY_ROW 30
#define DISPLAY_COL 80

typedef struct {
	/**
	 *  x x x x | x x x x
	 *   back   |   fore
	 */
	unsigned char color;
	char ascii;
} Display_mem_item;

typedef struct Display_mem{
	Display_mem_item* mem_base;
	/**
	 * cursor_row and cursor_col,
	 * point to the next display item
	 */
	int cursor_row, cursor_col;
	int tol_row, tol_col;
	int scannar_start_row;
	int multi_pages;
} Display_mem;

extern int scan_graphic;

void init_display_mem();
void graphic_card_scanner();
void write_to_graphic_card(Display_mem_item *tmp);
// void write_graphic_card(const char* in, char color);


#endif

