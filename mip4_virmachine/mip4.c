#include "os.h"
#include "utils.h"
#include "console.h"
#include "graphics.h"
#include "cpu.h"
#include "disk.h"
#include <pthread.h>
#include <fcntl.h>
mem_word *memory;
FILE *log_fp;
static void init_log_record()
{
    log_fp = fopen("logs", "w");
    if (log_fp == NULL)
    	exit(0);
}

static void echo_mip4()
{
	write_stdout("mip4 > ", F_WHITE, B_BLACK);
}

/**
 * 	0-3: text_seg start_offset(measure by bytes)
	4-7: text_seg size(no more than 16k)
	8-11: data_seg start_offset
	12-15: data_seg size(no more than 1k)
	16-19: debug_start_offset
	20-23: debug size

	notice: 写应用程序的时候stack不能超过1k
	写系统的时候所有程序加起来不能超过16k
 */

void vir_machine_init()
{
	init_log_record();
	init_console();
	init_display_mem();
	open_disk();
	/**
	 * initialize cpu
	 */
	int i;
	for (i = 0; i < CPU_REG_NUM; i++)
		R[i] = 0;
	memory = (mem_word *)malloc(sizeof(mem_word) * MEM_SIZE);
	/**
	 * TODO !
	 * INITIAL CP0
	 * CP0[32]...
	 */
}

void bootstrap()
{
	/**
	 * this is serial port communication
	 * might be look like this:
	 * 		jal load load_disk_sec
	 * 		j 	512
	 */
	load_disk_sec(memory + ONE_SECTOR * 1,
	              ONE_SECTOR * 3,
	              ONE_SECTOR * (MAX_KERNEL_TEXT_PAGE_NUM + MAX_KERNEL_DATA_PAGE_NUM));
}

int main(int argc, char *argv[])
{
	vir_machine_init();
	bootstrap();
	os_init();

	char order[800];
	int i;
	while (1) {
		echo_mip4();
		mips_want_get_string(order, 799);
		switch(order[0]) {
			case 'e':
				sys_exec(order + 2);
				break;
			case 'f':
				format_disk();
				write_stdout("successfully formats the disk!\n", F_WHITE, B_BLACK);
				break;
			case 'c':
				sys_create(order + 2);
				write_stdout("successfully create file\n", F_WHITE, B_BLACK);
				break;
			case 'w':  // copy the file to disk!
			{
				char *name = order + 2; // only one spaces !
				sys_create(name);
				int fid = sys_open(name);
				int obj_id = open(name, O_RDONLY);
				while (1) {
					int buf;
					if (read(obj_id, &buf, 1) == 0)
						break;
					char char_buf = (char)buf;
					sys_write(fid, &char_buf, 1);
				}
				sys_close(fid);
				write_stdout("successfully write file\n", F_WHITE, B_BLACK);
				break;
			}
		}
	}
	return 0;
}