#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "util.h"

int main(int argc, char** argv)
{
		UNUSED_ARGUMENT(argc);
		UNUSED_ARGUMENT(argv);

		int num = 5000000;
		int length = 164;
		char data[256];
		memset(data, 'X',length);

		struct timeval t1;
		struct timeval t2;

		gettimeofday(&t1, NULL);
		FILE* f;

		char filename[128];

		int i=0;
		sprintf(filename, "/storage/sflow/file_iotest.dat");
		for(; i<num; i++)
		{	
			if((f=fopen(filename, "a")) == NULL)
	        {
	                printf("%s\n", strerror(errno));
	                exit(1);
	        }
	        fwrite(data, length, 1, f);
	
        	fclose(f);
		}
		gettimeofday(&t2, NULL);
		unsigned int x = (t2.tv_sec-t1.tv_sec)*1000000 + abs(t2.tv_usec-t1.tv_usec);
		printf("\nTotal time elapsed: %u nanoseconds\n", x);
		printf("Average time for a single file operation: %.2f nanoseconds\n", x/((double)num));
		printf("Should be able to handle %u samples per second in terms of file I/O\n\n", 1000000/(x/num));
}
