#ifndef OS_H
#define OS_H

#include "global.h"
//////////////////////////////////////////////////////
#define BYTES_PER_FAT_ITEM 	4
#define ONE_PAGE 	   		((1) * (ONE_CLUSTER))

#define FAT_END 			0xffffffff
#define FAT_EMPTY 			0x0
#define MAX_FAT_TABLE		(ONE_SECTOR / 4 - 1)
int fat_table_array[MAX_FAT_TABLE];
int fat_table_size;
#define START_FAT_OFFSET 	66
/**
 * DIR REPRESENTATION IN ROOT DIR
 */
#define FILE_NAME_LEN 		15
#define END_OF_DIR 			0xff
typedef struct
{
	char dirName[FILE_NAME_LEN]; // 文件名
	char dirAttributes; // 文件属性
	int dirStartCluster;// 文件起始簇号
	int dirFileSize; // 表示文件的长度
} DirEntry; // size of this is 16 bytes!

DirEntry curDirEntry, newDirEntry; // rootDirEntry, oldDirEntry, ;
#define ROOT_CLUSTER 		2
/**
 * THE IDEA OF FILE!
 */
#define MAX_OPEN_FILE 		10
typedef struct
{
	int free_file_count; // initialize with MAX_OPEN_FILE
	int is_free[MAX_OPEN_FILE];
	DirEntry file_entry[MAX_OPEN_FILE];
	int fptr[MAX_OPEN_FILE];
	int rootDirOffset[MAX_OPEN_FILE];
} DirEntry_M;
DirEntry_M dir_entry_m;
typedef enum
{
	M_SEEK_SET,
	M_SEEK_CUR,
} FILE_POS;

#define KERNEL_BUF_SECTOR 	4
typedef struct
{
	int cluster_offset[KERNEL_BUF_SECTOR];
	char cluster_buffer[KERNEL_BUF_SECTOR][ONE_SECTOR];
	int cluster_is_dirty[KERNEL_BUF_SECTOR];
	int link_head, link_tail;
	int link[KERNEL_BUF_SECTOR];
} Kernel_buf;
Kernel_buf kernel_buf;
int cur_cluster_buf_offset;



#define TEXT_START 			0x00400
#define DATA_START 			0x10010
#define STACK_START			0x7ffff


#define KERNEL 				0
#define USER 				1

/**
 *  THE IDEA OF SIMPLE PROCESS
 */
#define MAX_PROCESS_NUM 	3
typedef struct
{
	int pid;
	int pc;
} PCB;
PCB pcb[MAX_PROCESS_NUM];
int cur_process = 0;
/**
 * define page related structure
 */
#define	MAX_TEXT_PAGE_NUM 			16
#define MAX_DATA_PAGE_NUM 			4
#define MAX_STACK_PAGE_NUM 			16
#define MAX_KERNEL_TEXT_PAGE_NUM 	64
#define MAX_KERNEL_DATA_PAGE_NUM	64
#define USER_MAX_PAGE_NUM 		(MAX_TEXT_PAGE_NUM + MAX_DATA_PAGE_NUM + MAX_STACK_PAGE_NUM)

#define MAX_PAGE_ITEM 		(MEM_SIZE / ONE_SECTOR)
typedef struct
{
	int vpn: 32; // 23
	int ppn: 32; // 23
} PageTableItem;
PageTableItem page_table[MAX_PROCESS_NUM][USER_MAX_PAGE_NUM];
int free_page_list[MAX_PAGE_ITEM];
int free_page_head;
int free_page_num;



//////////////////////////////////////////////////////

#define KERNEL_START 		0x80000
#define MAPPED_START 		0xc0000
#define DISPLAY_START 		0xd0000

int 		check_keyboard(int f);
mem_addr 	get_mapped_addr(mem_addr addr);
void 		write_stdout (char *msg, COLOR fore, COLOR back);
void  		mips_want_get_string(char *str, int max_char);

typedef enum
{
	CANNOT_FIND_FILE = 0,
	WROND_FILE_TYPE,
	TOO_MANY_OPEN_FILES,
	TEXT_SEGMENT_TOO_LARGE,
	DATA_SEGMENT_TOO_LARGE,
	NO_FREE_PAGE,
} Errno;
Errno errno;

/**
 * syscalls
 */
int sys_open(char *file_name);
void sys_lseek(int fid, int offset, FILE_POS file_pos);
int sys_read(int fid, char *buf, int size);
int sys_write(int fid, char *buf, int size);
void sys_create(char *file_name);
void sys_exit();
int sys_exec(char *file_name);
void sys_close(int fid);


void format_disk();
#endif