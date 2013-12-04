#include "keyboard.h"
#include "utils.h"

static Key_buffer key_buffer;

void init_key_buffer()
{
	key_buffer.buff_size = KEY_BUFFER_SIZE; // 16 byte
	key_buffer.key_ptr = 0;
	key_buffer.buff_base = (char *) xmalloc(sizeof(char) * key_buffer.buff_size);
}

int
check_input_available()
{
	fd_set fdset;
	struct timeval timeout;
	timeout.tv_sec = 0; // second
	timeout.tv_usec = 0; // 毫秒
	FD_ZERO (&fdset);
	FD_SET ((int) console_in.i, &fdset);
	// 有文件可读，返回大于1的书
	// 如果没有可读的文件，则根据timeout参数再判断是否超时，若超出
	// timeout的时间，select返回0，若发生错误返回负值。
	// 传入null表示不关心文件的读
	// int select(int maxfdp,fd_set *readfds,fd_set *writefds,
	// fd_set *errorfds,struct timeval *timeout);
	//    struct timeval* timeout是select的超时时间
	/* 若将NULL以形参传入，即不传入时间结构，就是将select置于阻塞状态，一定等到监视文件描述符集合中某
	个文件描述符发生变化为止；第二，若将时间值设为0秒0毫秒，就变成一个纯粹的非阻塞函数，不管文件描述符
	是否有变化，都立刻返回继续执行，文件无变化返回0，有变化返回一个正值；第三，timeout的值大于0，这就
	是等待的超时时间，即 select在timeout时间内阻塞，超时时间之内有事件到来就返回了，否则在超时后不管怎
	样一定返回，返回值同上述。*/
	return (select (sizeof (fdset) * 8, &fdset, NULL, NULL, &timeout));
}

/**
 * the os will call this when there is a key interrupt
 */
void write_to_key_buffer(char c)
{
	int ptr = key_buffer.key_ptr % key_buffer.buff_size;
	key_buffer.buff_base[ptr] = c;
	key_buffer.key_ptr = key_buffer.key_ptr + 1;
	log_file("...write key %c to key buffer, the key buffer size is %d...",
			 key_buffer.buff_base[ key_buffer.key_ptr ],
			 key_buffer.key_ptr);
}

char read_from_key_buffer() {
	int ptr = --key_buffer.key_ptr;
	if (ptr >= 0) {
		ptr = ptr % key_buffer.buff_size;
		log_file("...read from key buffer, the key is %c, now the size is %d",
				 key_buffer.buff_base[key_buffer.key_ptr],
				 key_buffer.key_ptr);
		return key_buffer.buff_base[ptr];
	}
	else
		log_file("...the key buffer is empty...");
	return 0;
}
