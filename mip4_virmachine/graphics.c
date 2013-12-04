#include "graphics.h"
#include "console.h"
#include "utils.h"
Display_mem display_mem;
int scan_graphic = 0;

void
init_display_mem() {
	display_mem.tol_col = DISPLAY_COL; // 80
	display_mem.tol_row = DISPLAY_ROW; // 30
	display_mem.mem_base = (Display_mem_item *) zmalloc(sizeof(Display_mem_item) *
														display_mem.tol_row * display_mem.tol_col);
	display_mem.scannar_start_row = 0;
	display_mem.multi_pages = 0;
}

void
write_to_graphic_card(Display_mem_item *tmp)
{
	if (tmp->ascii == '\n') {
		display_mem.cursor_row++;

		if (display_mem.cursor_row == display_mem.tol_row) {
			display_mem.multi_pages = 1;
			display_mem.cursor_row = 0;
		}
		if (display_mem.multi_pages == 1) {
			display_mem.scannar_start_row++;
			Display_mem_item *base = display_mem.mem_base + display_mem.cursor_row * display_mem.tol_col;
			int i;
			for (i = 0; i < display_mem.tol_col; i++)
				base[i].ascii = 0;
		}

		display_mem.cursor_col = 0;
	}
	else {
		if (display_mem.cursor_col == display_mem.tol_col) {
			display_mem.cursor_row++;
			if (display_mem.multi_pages)
				display_mem.scannar_start_row++;

			display_mem.cursor_col = 0;
		}
		Display_mem_item *base = display_mem.mem_base;
		base[ display_mem.cursor_row * display_mem.tol_col + display_mem.cursor_col ] = *tmp;
		display_mem.cursor_col++;
	}
	if (display_mem.scannar_start_row == display_mem.tol_row)
		display_mem.scannar_start_row = 0;
}

void
graphic_card_scanner() {
	while (1) {
		if (scan_graphic) {
			int tol = display_mem.tol_col * display_mem.tol_row;
			int i, j;
			clean_screen();
			int row = display_mem.scannar_start_row;
			for (i = 0; i < display_mem.tol_row; i++) {
				int off = row * display_mem.tol_col;
				for (j = 0; j < display_mem.tol_col; j++) {
					Display_mem_item *base = display_mem.mem_base + off + j;
					char fore = base->color & 0xf;
					char back = base->color >> 4;
					// write_direct_to_console("\e[3%dm\e[4%dm", fore, back);
					// write_direct_to_console("\e[3%dm", fore);

					if (0 == base->ascii)
						write_direct_to_console("%c", ' ');
					else
						write_direct_to_console("%c", base->ascii);
				}
				row++;
				if (row == display_mem.tol_row)
					row = 0;
			}
		}
		return;
	}
}

// TODO: FOR NOW THERE IS NOW STDOUT BUFFER !!!!!!!
// void write_graphic_card(const char* in, char color) {
// 	while (0 != *in) {
// 		char c = *in;
// 		in++;
// 		if (c == '\n') {
// 			display_mem.cursor_row++;
// 			display_mem.cursor_col = 0;
// 			continue;
// 		}
// 		if (display_mem.cursor_row == display_mem.tol_row) {
// 			int i = 0;
// 			Display_mem_item *mem_base_ptr = display_mem.mem_base;
// 			for (; i < display_mem.tol_row * display_mem.tol_col; i++) {
// 				*(mem_base_ptr)= *(mem_base_ptr + tol_row);
// 				mem_base_ptr++;
// 			}
// 			display_mem.cursor_row -= 1;
// 		}
// 		if (display_mem.cursor_col == display_mem.tol_col) {
// 			display_mem.cursor_row += 1;
// 			display_mem.cursor_col = 0;
// 			in--;
// 			continue;
// 		}
// 		{
// 			Display_mem_item d_m;
// 			d_m.color = color;
// 			d_m.ascii = c;
// 			*(display_mem.mem_base + display_mem.cursor_row * display_mem.tol_col + display_mem.cursor_col) = d_m;
// 		}
// 	}
// 	graphic_card_scanner();
// }