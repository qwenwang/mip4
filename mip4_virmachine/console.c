#include "console.h"

struct termios saved_console_out_state, saved_console_in_state;
Port console_in, console_out;

void init_console()
{
    console_out.f = stdout;
    console_out.i = 1;
    echo_off();
    not_carriage();
    // clean_screen();
    fflush(console_out.f);
}

/**
 * 关闭回显
 */
void echo_off()
{
    struct termios params;
    tcgetattr(console_out.i, &saved_console_out_state);
    params = saved_console_out_state;
    params.c_lflag &= ~ECHO; /* turn echo off */
    tcsetattr(console_out.i, TCSANOW, &params); /* set settings */
}
/**
 * 关闭回车
 */
void not_carriage()
{
    struct termios params;
    int ret;
    tcgetattr(console_in.i, &saved_console_in_state);
    params = saved_console_in_state;
      // params.c_iflag &= ~(ISTRIP|INLCR|ICRNL|IGNCR|IXON|IXOFF|INPCK|BRKINT|PARMRK);

      // /* Translate CR -> NL to canonicalize input. */
      // params.c_iflag |= IGNBRK|IGNPAR|ICRNL;
      // params.c_oflag = OPOST|ONLCR;
      // params.c_cflag &= ~PARENB;
      // params.c_cflag |= CREAD|CS8;h
      // params.c_lflag = 0;
      // params.c_cc[VMIN] = 1;
      // params.c_cc[VTIME] = 1;
    params.c_lflag &= ~(ICANON);
    params.c_cc[VMIN] = 1;
    params.c_cc[VTIME] = 0;
    ret = tcsetattr(console_in.i, TCSANOW, &params);
    if (ret < 0)
        log_file("%s %m", "Cannot set 'not carriage'");
}

/**
 * 下面这个函数是真正的输出,
 * 直接输出到console
 */
void
write_direct_to_console (char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    vfprintf (console_out.f, fmt, args);
    fflush (console_out.f);
    va_end (args);
}

/**
 * 清屏
 */
void
clean_screen()
{
    write_direct_to_console("%s", "\e[1H\e[2J\e[?25h");
    // write_direct_to_console("%s", "\e[2J\e[?25h");
}

