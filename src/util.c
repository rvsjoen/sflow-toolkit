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
	logmsg(LOGLEVEL_INFO, "Exiting gracefully");
	exit(r);
}

void exit_on_error() {
	if(!daemonize)
		disable_echo(false);
	logmsg(LOGLEVEL_ERROR, "Exiting on error");
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

void printSingleLineHex(unsigned char* pkt, uint32_t len){
	uint32_t i;
	for(i=0; i<len; i++){
		printf("%.2X", *pkt);
		pkt++;
	}
}

void printInHex(unsigned char* pkt, uint32_t len){
        printf("\n\tHEX dump\n\t");
	uint32_t j=0;
        uint32_t i;
        for(i=0; i<len; i++){
		if(j++%2 == 0)
			printf(" ");

                printf("%.2X", *pkt);
                pkt++;
		j++;
		if((i+1)%30 == 0)
			printf("\n\t");
        }
        printf("\n\n");
}

void num_to_ip(uint32_t num, char* buf){
	sprintf(buf, "%i.%i.%i.%i",
		((num & 0xff000000) >> 24),
		((num & 0x00ff0000) >> 16),
		((num & 0x0000ff00) >> 8),
		(num & 0x000000ff)
	);
}
