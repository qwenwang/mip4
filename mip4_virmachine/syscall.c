#include "syscall.h"
#include "cpu.h"
#include "os.h"
#include "utils.h"
int
do_syscall ()
{
    switch (R[REG_V0])
    {
        case PRINT_INT_SYSCALL:
            // write_output (console_out, "%d", R[REG_A0]);
            break;

        case PRINT_STRING_SYSCALL:

            write_stdout("haha", F_WHITE, B_BLACK);
            // write_output (console_out, "%s", mem_reference (R[REG_A0]));
            break;

        case READ_INT_SYSCALL:
        {
          	static char str [256];
          	// read_input (str, 256);
          	R[REG_RES] = atol (str);
          	break;
        }


        case READ_STRING_SYSCALL:
        {
          	// read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
          	// data_modified = 1;
          	break;
        }

        case PRINT_CHARACTER_SYSCALL:
            // write_output (console_out, "%c", R[REG_A0]);
            break;

        case READ_CHARACTER_SYSCALL:
        {
          	static char str [2];

          	// read_input (str, 2);
          	if (*str == '\0')
                *str = '\n';      /* makes xspim = spim */
          	R[REG_RES] = (long) str[0];
            break;
        }

        case EXIT_SYSCALL:
            // spim_return_value = 0;
            return 0;

        case EXIT2_SYSCALL:
            // spim_return_value = R[REG_A0];	/* value passed to spim's exit)call */
            return 0;

        case OPEN_SYSCALL:
        // {
        //   	R[REG_RES] = open(mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
        //   	break;
        // }

        case READ_SYSCALL:
        // {
        //   	/* Test if address is valid */
        //   	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
        //   	R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
        //   	data_modified = 1;
        //   	break;
        // }

        case WRITE_SYSCALL:
        // {
        // 	/* Test if address is valid */
        //   /* ?? */
        // 	  (void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
        //     R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
        // 	  break;
        // }
        case CLOSE_SYSCALL:
            break;
        // {
        //     R[REG_RES] = close(R[REG_A0]);
        //     break;
        // }

        default:
          run_error ("Unknown system call: %d\n", R[REG_V0]);
          break;
    }

    return 1;
}


