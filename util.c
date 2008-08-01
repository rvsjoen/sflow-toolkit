#define  _GNU_SOURCE

#include "util.h"

tcflag_t tflags = 0;

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  disable_echo
 *  Description:  This function enables/disables echo in the terminal
 * =====================================================================================
 */
void disable_echo(bool b){
    int res = open((char*)ctermid(NULL), O_WRONLY);
    if(res != -1){
        struct termios tios;
        tcgetattr(res, &tios);
        if(b){
            if(tflags == 0){
                tflags = tios.c_lflag;
                tios.c_lflag = tflags & ~ECHO;
            }
        } else {
            if(tflags != 0){
                tios.c_lflag = tflags;
                tflags = 0;
            }
        }
        tcsetattr(res, TCSADRAIN, &tios);
        close(res);
    }
}

void exit_collector(int r){
    disable_echo(false);
	exit(r);
}
