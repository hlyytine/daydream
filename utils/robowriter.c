#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <ddlib.h>
#include <dd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <config.h>
#include <ddcommon.h>

static void getmsgptrs(void);
static int setmsgptrs(void);
static void showhelp(void);

static int dconf;
static int dbase;
static int msgfd;
static char *receiver="\0";
static char *author="Robowriter";
static char *subject="--unknown--";
static char *mfile=0;
static int private=0;
static FILE *inf;
static FILE *outf;

static struct DayDream_Conference *confd;
static struct DayDream_MsgBase *based;

static unsigned short lowest, highest;

int main(int argc, char *argv[])
{
	char *cp;
	struct DayDream_Message ddm;
	char ebuf[1024];
	
	while (--argc) {
	cp = *++argv;
		if (*cp == '-') {
			while( *++cp) {
				switch(*cp) {
				case 'p':
					private=1; break;
				case 'r':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					receiver=*++argv;
					break;
				case 'f':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					author=*++argv;
					break;
				case 's':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					subject=*++argv;
					break;
				case 'c':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					dconf=atoi(*++argv);
					break;
				case 'b':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					dbase=atoi(*++argv);
					break;
				}
				
			}
		} else {
			mfile=cp;
		}	
	}
	if (!dconf || !dbase || !mfile) {
		showhelp();
		exit(1);
	}
	confd=dd_getconf(dconf);
	if (!confd) {
		printf("Can't get conference!\n");
		exit(0);
	}
	based=dd_getbase(dconf,dbase);
	if (!based) {
		printf("Can't get messagebase!\n");
		exit(0);
	}
	inf=fopen(mfile,"r");
	if (!inf) {
		printf("Can't open message file!\n");
		exit(0);
	}
	memset(&ddm,0,sizeof(struct DayDream_Message));
	if (private) ddm.MSG_FLAGS |= (1L<<0);
	strncpy(ddm.MSG_AUTHOR,author,25);
	if (!strcasecmp(receiver,"All")) {
		*ddm.MSG_RECEIVER=0;
	} else if (!strcasecmp(receiver,"EAll")) {
		*ddm.MSG_RECEIVER=255;
	} else {
		strncpy(ddm.MSG_RECEIVER,receiver,25);
	}
	strncpy(ddm.MSG_SUBJECT,subject,25);
	ddm.MSG_CREATION=time(0);
	getmsgptrs();
	highest++;
	ddm.MSG_NUMBER=highest;
	if (setmsgptrs()) {
		char outbuf[4096];
		sprintf(ebuf, "%s/messages/base%3.3d/msgbase.dat", confd->CONF_PATH, based->MSGBASE_NUMBER);

		if ((msgfd=open(ebuf,O_RDWR|O_CREAT,0664)) < 0) {
			printf("*FATAL* error.. Can't write message!\n\n");
			exit(1);
		}

		lseek(msgfd,0,SEEK_END);
		if (toupper(based->MSGBASE_FN_FLAGS)=='E') {
			ddm.MSG_FN_ORIG_ZONE=based->MSGBASE_FN_ZONE;
			ddm.MSG_FN_ORIG_NET=based->MSGBASE_FN_NET;
			ddm.MSG_FN_ORIG_NODE=based->MSGBASE_FN_NODE;
			ddm.MSG_FN_ORIG_POINT=based->MSGBASE_FN_POINT;
			ddm.MSG_FLAGS |= (1L<<2);
		}
		write(msgfd,&ddm,sizeof(struct DayDream_Message));
		close(msgfd);
		
		sprintf(ebuf, "%s/messages/base%3.3d/msg%5.5d", confd->CONF_PATH, based->MSGBASE_NUMBER, ddm.MSG_NUMBER);
	
		outf=fopen(ebuf,"w");
		if (!outf) {
			printf("*FATAL* error.. Can't write message!\n\n");
			exit(1);
		}
		if (toupper(based->MSGBASE_FN_FLAGS)=='E') {
			char ub[128];
			char ebuf[1024];
			int uq;
		
			strcpy(ub,based->MSGBASE_FN_TAG);
			strupr(ub);
			sprintf(ebuf,"AREA:%s\n",ub);
			fputs(ebuf,outf);
			if ((uq=dd_getfidounique())) {
				sprintf(ebuf,"\001MSGID: %d:%d/%d.%d %8.8x\n",based->MSGBASE_FN_ZONE,based->MSGBASE_FN_NET,based->MSGBASE_FN_NODE,based->MSGBASE_FN_POINT,uq);
				fputs(ebuf,outf);
			}
		}

		while(fgets(outbuf,4096,inf)) {
			fputs(outbuf,outf);
		}
		if (toupper(based->MSGBASE_FN_FLAGS)=='E') {
			fprintf(outf,"\n--- DayDream BBS/Linux %s\n * Origin: %s (%d:%d/%d)\nSEEN-BY: %d/%d\n",versionstring,based->MSGBASE_FN_ORIGIN,based->MSGBASE_FN_ZONE,based->MSGBASE_FN_NET,based->MSGBASE_FN_NODE,based->MSGBASE_FN_NET,based->MSGBASE_FN_NODE);
		}					
		fclose(outf);
	
	}
	fclose(inf);
	return 0;
}

static void showhelp(void)
{
	printf("DD-Robowriter v1.0 - written by Antti Häyrynen\n\n");
	printf("Usage: robowriter [-f from] [-r receiver] [-s subject] [-c conference]\n");
	printf("                  [-b base] [-p] file...\n\n -p == private\n");
	printf(" Note that you must supply at least -c, -b and file\n");
}

static void getmsgptrs(void)
{
	int msgfd;
	struct DayDream_MsgPointers ptrs;
	
	char gmpbuf[300];
	sprintf(gmpbuf, "%s/messages/base%3.3d/msgbase.ptr", confd->CONF_PATH, based->MSGBASE_NUMBER);

	if ((msgfd=open(gmpbuf,O_RDONLY)) < 0) {
		highest=0;
		lowest=0;
		return;
	}

	read(msgfd,&ptrs,sizeof(struct DayDream_MsgPointers));
	close(msgfd);
	highest=ptrs.msp_high;
	lowest=ptrs.msp_low;	
}

static int setmsgptrs(void)
{
	int msgfd;
	struct DayDream_MsgPointers ptrs;
	
	char gmpbuf[300];
	sprintf(gmpbuf, "%s/messages/base%3.3d/msgbase.ptr", confd->CONF_PATH, based->MSGBASE_NUMBER);

	if ((msgfd=open(gmpbuf,O_RDWR|O_CREAT,0664)) < 0) {
		printf("*FATAL* error.. Can't write message pointers!\n\n");
		return 0;
	}

	ptrs.msp_high=highest;
	ptrs.msp_low=lowest;
	write(msgfd,&ptrs,sizeof(struct DayDream_MsgPointers));
	close(msgfd);
	return 1;
}
