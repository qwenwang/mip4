#include "os.h"
#include "utils.h"
#include "console.h"
#include "keyboard.h"
#include "graphics.h"
#include "cpu.h"
#include <fcntl.h>
#include <unistd.h>

static int fat_table_array[MAX_FAT_TABLE];
static int fat_table_size;
static DirEntry curDirEntry, newDirEntry; // rootDirEntry, oldDirEntry, ;
static DirEntry_M dir_entry_m;
static Kernel_buf kernel_buf;

int cur_process = 0;
int page_table[MAX_PROCESS_NUM][USER_MAX_PAGE_NUM];
static int old_free_page;
static int next_free_page;

static int cur_cluster_buf_offset;
static Errno errno;


static int allocate_page(int fid, int size, int page_start, int segment_start);
// 只要使用前几个字符即可使用命令
static int
str_prefix (char *s1, char *s2, int min_match)
{
  	for ( ; *s1 == *s2 && *s1 != '\0'; s1 ++, s2 ++)
  		min_match --;
  	return (*s1 == '\0' && min_match <= 0);
}

static char stdin_buff[ONE_PAGE];
static int stdin_ptr;
static int stdin_end;
static int stdin_size = ONE_PAGE;
static void write_key_to_istream()
{
	char c = read_from_key_buffer();
	if (c)
		stdin_buff[stdin_end++ % ONE_PAGE] = c;
}

static char read_key_from_istream()
{
	if (stdin_ptr < stdin_end) {
		int ptr = stdin_ptr % stdin_size;
		stdin_ptr++;
		return stdin_buff[ptr];
	}
	return 0;
}

extern reg_word CP0[32];

void
write_stdout (char *msg, COLOR fore, COLOR back)
{
	Display_mem_item tmp;
	while (*msg) {
		tmp.color = back | fore;
		tmp.ascii = *msg;
		write_to_graphic_card(&tmp);
		msg++;
	}
	scan_graphic = 1;
	graphic_card_scanner();
}


// 检查键盘输入并回显
// This is hardware part
int check_keyboard(int f)
{
	while (1) {
		if (check_input_available())
		{
			char buf[2];
			// buf[0] = getchar();
    		read ((int) console_in.i, &buf[0], 1);
			buf[1] = '\0';
			write_to_key_buffer(buf[0]);
			write_key_to_istream();
			write_stdout(buf, F_WHITE, B_BLACK);
			// 写到文件流中, 根据现在谁在top上
			return 1;
		}
		if (!f)
			return 0;
	}
}

void
mips_want_get_string(char order[], int max_char) {
	char c;
	int i = 0;
	do {
		check_keyboard(1);
		c = read_key_from_istream();
		if (!c) {
			log_file("unknown error");
			exit(1);
		}
		if (c == '\n')
			break;
		order[i++] = c;
	} while (i != max_char);
	order[i] = '\0';
	log_file("done with get_string");
}

void write_cur_to_disk()
{
	write_to_disk(kernel_buf.cluster_offset[cur_cluster_buf_offset],
	              kernel_buf.cluster_buffer[cur_cluster_buf_offset],
	              ONE_CLUSTER);
}

int is_cluster_in_buf(int cluster_offset)
{
	int i;
	for (i = 0; i < KERNEL_BUF_SECTOR; i++) {
		if (kernel_buf.cluster_offset[i] == cluster_offset)
			return i;
	}
	return -1;
}

void os_helper_get_cluster(int cluster_num)
{
	cur_cluster_buf_offset = is_cluster_in_buf(cluster_num);
	if (-1 == cur_cluster_buf_offset) {
		cur_cluster_buf_offset = (kernel_buf.next_free++) % KERNEL_BUF_SECTOR;
		load_disk_sec(kernel_buf.cluster_buffer[cur_cluster_buf_offset], cluster_num * ONE_CLUSTER, 1 * ONE_CLUSTER);
		kernel_buf.cluster_offset[cur_cluster_buf_offset] = cluster_num;
	}
}

int os_helper_get_next_cluster(int cluster_offset)
{
	int item_per_fat = ONE_CLUSTER / BYTES_PER_FAT_ITEM;
	int fat_table_num = cluster_offset / item_per_fat;
	int fat_cluster = fat_table_array[fat_table_num];
	os_helper_get_cluster(fat_cluster);
	return *( (int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset]) + cluster_offset % item_per_fat);
}

int os_helper_find_free_fat_entry_and_set()
{
	int i;
	int fat_item = ONE_CLUSTER / BYTES_PER_FAT_ITEM;

	for (i = 0; i < fat_table_size; i++) {
		os_helper_get_cluster(fat_table_array[i]);
		char *cluster_ptr = kernel_buf.cluster_buffer[cur_cluster_buf_offset];
		int j = 0;
		if (i == 0) {
			j = START_FAT_OFFSET;
			cluster_ptr += BYTES_PER_FAT_ITEM * j;
		}
		for (; j < fat_item; j++) {
			if (*(int *)cluster_ptr == FAT_EMPTY) {
				*(int *)cluster_ptr = FAT_END;
				write_cur_to_disk();
			/*	if (j == fat_item - 1) {
					fat_table_size++;
					fat_table_array[i + 1] = i * ONE_CLUSTER + j;
					*
					 * initialize the new fat table!

					os_helper_get_cluster(fat_table_array[i + 1]);
				} else
				*/
				return i * fat_item + j;
			}
			cluster_ptr += BYTES_PER_FAT_ITEM;
		}
	}
}

void os_helper_set_fat_item(int item_1, int item_2)
{
	int item_per_fat = ONE_CLUSTER / BYTES_PER_FAT_ITEM;
	int fat_table_num = item_1 / item_per_fat;
	int fat_cluster = fat_table_array[fat_table_num];
	os_helper_get_cluster(fat_cluster);
	*(int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset] + item_1 % item_per_fat * BYTES_PER_FAT_ITEM) = item_2;
	write_cur_to_disk();
	// kernel_buf.cluster_is_dirty[cur_cluster_buf_offset] = 1;
}

void switch_state(int user)
{}



int get_free_page()
{
	if (next_free_page < MAX_PAGE_ITEM)
		return next_free_page++;
	return -1;
}

int allocate_page(int fid, int size, int page_start, int segment_start)
{
	int to_read;
	int page = (size - 1) / ONE_PAGE;
	int i;
	for (i = 0; i <= page; i++) {
		int free_page = get_free_page();
		if (-1 == free_page) {
			errno = NO_FREE_PAGE;
			exit(1);
		}
		int page_item = free_page;
		page_table[cur_process][i + page_start] = page_item;
		if (size >= ONE_PAGE)
			to_read = ONE_PAGE;
		else
			to_read = size;
		sys_read(fid, (char *)memory + free_page * ONE_PAGE, to_read);
		size -= to_read;
	}
	return 1;
}

void os_init()
{
	/* initialize the kernel buffer */
	int i;
	kernel_buf.next_free = 0;
	for (i = 0; i < KERNEL_BUF_SECTOR; i++) {
		kernel_buf.cluster_offset[i] = -1;
		kernel_buf.cluster_is_dirty[i] = 0;
	}
	/* load fat list */
	os_helper_get_cluster(0); // fat list
	fat_table_size = *(int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset]);

	for (i = 0; i < fat_table_size; i++)
		fat_table_array[i] = *(int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset] + 4 * (i + 1));
	/* load the root dir */
	os_helper_get_cluster(ROOT_CLUSTER);
	curDirEntry.dirStartCluster = *(int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset] + 16);
	curDirEntry.dirFileSize 	= *(int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset] + 20);
	/* initial the page list */
	next_free_page = 65 + STACK_PAGE_NUM;
	/**/
	dir_entry_m.free_file_count = MAX_OPEN_FILE;
	for (i = 0; i < MAX_OPEN_FILE; i++)
		dir_entry_m.is_free[i] = 1;

	init_key_buffer();

	// write_stdout("welcome to mips world, now start to enjoy\n", F_WHITE, B_BLACK);
}

int os_helper_open_one_file(char *dir_name)
{
	int dir_entry_len = sizeof(DirEntry);
	int dir_entry_num = ONE_CLUSTER / dir_entry_len;
	int cluster_num = (curDirEntry.dirFileSize + ONE_CLUSTER - 1) / ONE_CLUSTER;
	int k, i;
	int next_cluster = curDirEntry.dirStartCluster;
	int total_item = curDirEntry.dirFileSize / dir_entry_len;
	int item_count = 0;
	for (k = 0; k < cluster_num; k++) {
		os_helper_get_cluster(next_cluster);
		char *cluster_start = kernel_buf.cluster_buffer[cur_cluster_buf_offset];
		for (i = 0; i < dir_entry_num; ) {
			int j;
			if (item_count++ == total_item) {
				errno = CANNOT_FIND_FILE; //
				return 0;
			}
			for (j = 0; j < FILE_NAME_LEN; j++) {
				if (dir_name[j] && dir_name[j] == cluster_start[j])
					continue;
				if (dir_name[j])
					break;
				strncpy(newDirEntry.dirName, cluster_start, FILE_NAME_LEN);
				newDirEntry.dirAttributes = *(char *)(cluster_start + FILE_NAME_LEN);
				newDirEntry.dirStartCluster = *(int *)(cluster_start + FILE_NAME_LEN + 1);
				newDirEntry.dirFileSize = *(int *)(cluster_start + FILE_NAME_LEN + 1 + sizeof(int));
				return k * dir_entry_num + i;
			}
			cluster_start += dir_entry_len;
			i++;
		}
		next_cluster = os_helper_get_next_cluster(next_cluster);
	}
}

int sys_open(char *file_name)
{
	if (0 == dir_entry_m.free_file_count) {
		errno = TOO_MANY_OPEN_FILES;
		return -1;
	}
	int entry_off;
	if (!(entry_off = os_helper_open_one_file(file_name)))
		return -1;

	int i;
	for (i = 0; i < MAX_OPEN_FILE; i++) {
		if (dir_entry_m.is_free[i] == 1) {
			dir_entry_m.file_entry[i] = newDirEntry;
			dir_entry_m.free_file_count -= 1;
			dir_entry_m.fptr[i] = 0;
			dir_entry_m.is_free[i] = 0;
			dir_entry_m.rootDirOffset[i] = entry_off;
			return i;
		}
	}
}

void sys_lseek(int fid, int offset, FILE_POS file_pos)
{
	if (file_pos == M_SEEK_SET)
		dir_entry_m.fptr[fid] = offset;
	else if (file_pos == M_SEEK_CUR)
		dir_entry_m.fptr[fid] += offset;
}

int sys_read(int fid, char *buf, int size)
{
	if (0 == size)
		return 0;
	DirEntry file = dir_entry_m.file_entry[fid];
	int fptr = dir_entry_m.fptr[fid];
	if (fptr < file.dirFileSize) {
		int cluster_num = fptr / ONE_CLUSTER;
		int offset_in_cluster = fptr % ONE_CLUSTER;
		int to_read = ONE_CLUSTER - offset_in_cluster;

		int next_cluster = file.dirStartCluster;
		while (cluster_num--)
			next_cluster = os_helper_get_next_cluster(next_cluster);
		os_helper_get_cluster(next_cluster);
		if (size < to_read)
			to_read = size;
		int i;
		for (i = 0; i < to_read; i++) {
			// int mm = hardware_mem_map((int)buf);
			char *mm = buf;
			*(char *)mm = kernel_buf.cluster_buffer[cur_cluster_buf_offset][offset_in_cluster++];
			fptr++;
			buf++;
		}
		int left = size - to_read;
		while (left > 0) {
			next_cluster = os_helper_get_next_cluster(next_cluster);
			os_helper_get_cluster(next_cluster);
			if (left <= ONE_CLUSTER) {
				int i;
				for (i = 0; i < left; i++) {
					// int mm = hardware_mem_map((int)buf);
					char *mm = buf;
					*(char *)mm = kernel_buf.cluster_buffer[cur_cluster_buf_offset][i++];
					fptr++;
					buf++;
				}
			}
			left -= ONE_CLUSTER;
		}
		dir_entry_m.fptr[fid] = fptr;
	}
	else
		return 0;
}

int sys_write(int fid, char *buf, int size)
{
	if (0 == size)
		return 0;
	int fptr = dir_entry_m.fptr[fid];
	DirEntry file = dir_entry_m.file_entry[fid];
	int cluster_num = fptr / ONE_CLUSTER;
	int offset_in_cluster = fptr % ONE_CLUSTER;
	int to_write_byte = ONE_CLUSTER - offset_in_cluster;

	int next_cluster = file.dirStartCluster;
	while (cluster_num--)
		next_cluster = os_helper_get_next_cluster(next_cluster);
	os_helper_get_cluster(next_cluster);
	int i;
	for (i = 0; i < to_write_byte; i++) {
		if (size-- > 0) {
			/* here doesn't fully simulated*/
			kernel_buf.cluster_buffer[cur_cluster_buf_offset][offset_in_cluster++] = *buf++;
			fptr++;
		} else
			break;
	}
	write_cur_to_disk();
	int pre_cluster = next_cluster;
	while (size > 0) {
		next_cluster = os_helper_get_next_cluster(pre_cluster);
		if (next_cluster == FAT_END) {
			/* fat reach the end of file size */
			next_cluster = os_helper_find_free_fat_entry_and_set();
			os_helper_set_fat_item(pre_cluster, next_cluster);
			pre_cluster = next_cluster;
		}
		os_helper_get_cluster(next_cluster);
		int i;
		for (i = 0; i < ONE_CLUSTER; i++) {
			if (size-- > 0) {
				kernel_buf.cluster_buffer[cur_cluster_buf_offset][i] = *buf++;
				fptr++;
			} else
				break;
		}
		write_cur_to_disk();
	}
	if (fptr > file.dirFileSize)
		file.dirFileSize = fptr;
	dir_entry_m.fptr[fid] = fptr;

	/*
	 * new fat table item
	 */
	if (fptr % ONE_CLUSTER == 0) {
		next_cluster = os_helper_find_free_fat_entry_and_set();
		os_helper_set_fat_item(pre_cluster, next_cluster);
	}
	/*
	 * modify file size
	 */
	dir_entry_m.file_entry[fid] = file;
	int dir_item = ONE_CLUSTER / sizeof(DirEntry);
	cluster_num = dir_entry_m.rootDirOffset[fid] / dir_item;
	offset_in_cluster = dir_entry_m.rootDirOffset[fid] % dir_item;
	next_cluster = curDirEntry.dirStartCluster;
	while (cluster_num--)
		next_cluster = os_helper_get_next_cluster(next_cluster);
	os_helper_get_cluster(next_cluster);
	*(int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset] + offset_in_cluster * sizeof(DirEntry) + 20) = file.dirFileSize;
	write_cur_to_disk();
}

void sys_create(char *file_name)
{
	int cluster_num = (curDirEntry.dirFileSize - 1) / ONE_CLUSTER;
	int last_item_offset = curDirEntry.dirFileSize % ONE_CLUSTER;
	int next_cluster = curDirEntry.dirStartCluster;
	while (cluster_num--)
		next_cluster = os_helper_get_next_cluster(next_cluster);
	os_helper_get_cluster(next_cluster);
	char *file = kernel_buf.cluster_buffer[cur_cluster_buf_offset] + last_item_offset;
	int i = 0;
	kernel_buf.cluster_is_dirty[cur_cluster_buf_offset] = 1;
	while (i < FILE_NAME_LEN && file_name[i]) {
		file[i] = file_name[i];
		i++;
	}
	file[i] = '\0';
	i = 15;
	file[i] = 0;
	*(int *)(file + i + 1) = os_helper_find_free_fat_entry_and_set();
	*(int *)(file + i + 5) = 0; // file_size
	os_helper_get_cluster(next_cluster);
	write_cur_to_disk();
	kernel_buf.cluster_is_dirty[cur_cluster_buf_offset] = 0;
	curDirEntry.dirFileSize += sizeof(DirEntry);
	os_helper_get_cluster(ROOT_CLUSTER);
	*(int *)(kernel_buf.cluster_buffer[cur_cluster_buf_offset] + 20) = curDirEntry.dirFileSize;
	write_cur_to_disk();
}

void sys_exit()
{
	int i;
		next_free_page = old_free_page;

	cur_process--;
	switch_state(USER);
}

int sys_exec(char *file_name)
{
	old_free_page = next_free_page;
	int fid = sys_open(file_name);
	int exec_file_header[4];
	/* here we have to save the address of EPC */
	sys_read(fid, (char *)exec_file_header, 4 * sizeof(int));
	if (exec_file_header[1] > MAX_TEXT_PAGE_NUM * K) {
		errno = TEXT_SEGMENT_TOO_LARGE;
		return -1;
	}
	if (exec_file_header[3] > MAX_DATA_PAGE_NUM * K) {
		errno = DATA_SEGMENT_TOO_LARGE;
		return -1;
	}
	cur_process++;
	int i;
	for (i = 0; i < USER_MAX_PAGE_NUM; i++)
		page_table[cur_process][i] = 0;

	/**
	 * allocated available page
	 */

	int text_size = exec_file_header[1];
	int data_size = exec_file_header[3];
	sys_lseek(fid, exec_file_header[0], M_SEEK_SET);
	allocate_page(fid, text_size, 0, TEXT_START);
	sys_lseek(fid, exec_file_header[2], M_SEEK_SET);
	allocate_page(fid, data_size, MAX_TEXT_PAGE_NUM, DATA_START);
	run(TEXT_START, 1000);
}

void sys_close(int fid)
{
	dir_entry_m.is_free[fid] = 0;
	dir_entry_m.free_file_count++;
}

extern int disk_fid;
void format_disk()
{
	/**
	 * like create a new file;
	 */
	FILE *fp = fopen("disk", "w");
	fclose(fp);

	disk_fid = open("disk", O_RDWR);
	DirEntry root;
	root.dirName[0] = '/';
	root.dirName[1] = '\0';
	root.dirStartCluster = ROOT_CLUSTER;
	root.dirFileSize = sizeof(DirEntry);
	lseek(disk_fid, ROOT_CLUSTER * ONE_SECTOR, SEEK_SET);
	write(disk_fid, &root, sizeof(DirEntry));

	int a[ONE_SECTOR / BYTES_PER_FAT_ITEM];
	int i;
	for (i = 0; i < START_FAT_OFFSET; i++) {
		a[i] = -1;
	}
	for (; i < ONE_SECTOR / BYTES_PER_FAT_ITEM; i++)
		a[i] = 0;
	lseek(disk_fid, 1 * ONE_SECTOR, SEEK_SET);
	write(disk_fid, &a, sizeof(a));

	lseek(disk_fid, 0, SEEK_SET);
	int init_fat_size = 1;
	write(disk_fid, &init_fat_size, sizeof(init_fat_size));
	int init_fat_cluster = 1;
	write(disk_fid, &init_fat_cluster, sizeof(init_fat_cluster));
	// close(disk_fid);
}