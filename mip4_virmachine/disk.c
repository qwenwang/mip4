#include "disk.h"
#include <fcntl.h>
int disk_fid;

void load_disk_sec(void *des_off, int src_off, int size)
{
	lseek(disk_fid, src_off, SEEK_SET);
	read(disk_fid, des_off, size);
}

void write_to_disk(int disk_cluster, void *mem, int size)
{
	lseek(disk_fid, disk_cluster * ONE_CLUSTER, SEEK_SET);
	write(disk_fid, mem, size);
}

void open_disk()
{
	disk_fid = open("disk", O_RDWR);
}