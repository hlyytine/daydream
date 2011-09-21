#include <stdio.h>
#include <stdlib.h>

#include <ddlib.h>

struct dif *d;

int main(int argc, char *argv[])
{
	if (argc==1) {
		printf("This program requires Microsoft Windows!\n");
		exit(1);
	}
	d=dd_initdoor(argv[1]);
	if (d==0) {
		printf("Couldn't find socket!\n");
		exit(1);
	}
	sleep(2);
	
	dd_sendstring(d, "Olet SILMÄ!!!!!!\n");
	dd_close(d);
	
	return 0;
}
