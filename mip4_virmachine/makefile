objects = mip4.o os.o syscall.o utils.o \
          keyboard.o graphics.o cpu.o console.o disk.o
global_h = global.h
os_h = os.h
syscall_h = syscall.h
utils_h = utils.h
keyboard_h = keyboard.h
cpu_h = cpu.h
console_h = console.h
graphics_h = graphics.h
CC = gcc -g3 -m32
mip4:$(objects)
	$(CC) -o mip4 -lpthread $(objects)

mip4.o: mip4.c $(global_h)
	$(CC) -c -DDEGUG mip4.c

os.o:os.c $(os_h) $(utils_h) $(console_h) $(keyboard_h) $(graphics_h)
	$(CC) -c os.c

utils.o:utils.c $(utils_h)
	$(CC) -c utils.c

syscall.o: 	syscall.c $(syscall_h)
	$(CC) -c syscall.c

keyboard.o: keyboard.c $(keyboard_h)
	$(CC) -c keyboard.c

graphics.o: graphics.c $(graphics_h)
	$(CC) -c graphics.c

cpu.o: 	cpu.c $(cpu_h)
	$(CC) -c cpu.c

console.o: console.c $(console_h)
	$(CC) -c console.c

disk.o: disk.c disk.h
	$(CC) -c disk.c
clean:
	rm mip4 $(objects)
