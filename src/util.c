#define  _GNU_SOURCE

#include "logger.h"
#include "util.h"

tcflag_t tflags = 0;
bool daemonize	= true;

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
	logmsg(LOGLEVEL_DEBUG, "Exiting gracefully");
	exit(r);
}

void exit_on_error() {
	if(!daemonize)
		disable_echo(false);
	logmsg(LOGLEVEL_DEBUG, "Exiting on error");
	exit(1);
}

void daemonize_me() {
	pid_t pid, sid;
	if ( getppid() == 1 ) return;

	// Fork off the parent process
	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);

	// If we got a good PID, then we can exit the parent process
	if (pid > 0)
		exit(EXIT_SUCCESS);

	// Change the file mode mask
	//umask(0);

	// Create a new SID for the child process
	sid = setsid();
	if (sid < 0)
		exit(EXIT_FAILURE);

	// Change the current working directory
	if ((chdir("/")) < 0)
		exit(EXIT_FAILURE);

	// Close out the standard file descriptors
	freopen( "/dev/null", "r", stdin);
	freopen( "/dev/null", "w", stdout);
	freopen( "/dev/null", "w", stderr);
}

