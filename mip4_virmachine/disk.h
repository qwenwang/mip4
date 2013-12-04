
#ifndef DISK_H
#define DISK_H
#include "global.h"

void load_disk_sec(void *des_off, int src_off, int size);
void write_to_disk(int disk_cluster, void *mem, int size);
void open_disk();

/**
 * NOT USED
 */
/*enum PageInfo {
	max_kernel_page_num = 16,
	max_mapped_page_num = 1,
	max_display_page_num = 1,
};*/

#endif