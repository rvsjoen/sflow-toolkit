#ifndef __util_h__
#define __util_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

void disable_echo(bool b);
void exit_collector(int r);

#endif
