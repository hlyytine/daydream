#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <ddlib.h>
#include <ddcommon.h>

/*
	DayDream Door Library - written by Antti Häyrynen
 */

void writedm(struct dif *);

struct DayDream_Conference *__confdatas = 0;

struct dif *dd_initdoor(char *node)
{
	struct dif *mydif;
	struct sockaddr_un socknfo;
	char portname[80];

	mydif = (struct dif *) malloc(sizeof(struct dif));
	snprintf(portname, sizeof portname, "%s/dd_door%s", DDTMP, node);
	strlcpy(socknfo.sun_path, portname, sizeof(socknfo.sun_path));
	socknfo.sun_family = AF_UNIX;

	mydif->dif_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (mydif->dif_sockfd == -1) {
		free(mydif);
		return 0;
	}
	if (connect (mydif->dif_sockfd, (struct sockaddr *) &socknfo,
		sizeof(socknfo)) == -1) {
		close(mydif->dif_sockfd);
		free(mydif);
		return 0;
	}
	signal(SIGHUP, SIG_IGN);

	return mydif;
}

void dd_close(struct dif *d)
{
	if (__confdatas) 
		free(__confdatas);
	d->dif_dm.ddm_command = 1;
	writedm(d);
}

void freecdatas(void)
{
	free(__confdatas);
}

struct DayDream_Conference *dd_getconfdata(void)
{
	struct stat fib;
	int datafd;
	char *s;
	if (!__confdatas) {
		char tbuf[1024];
		char *home;

		if ((home = getenv("DAYDREAM")) == NULL)
			return NULL;
		snprintf(tbuf, sizeof tbuf, "%s/data/conferences.dat",
			 home);
		if ((datafd = open(tbuf, O_RDONLY)) == -1)
			return NULL;

		fstat(datafd, &fib);

		__confdatas = (struct DayDream_Conference *) 
			malloc(fib.st_size + 2);
		read(datafd, __confdatas, fib.st_size);
		close(datafd);
		s = (char *) __confdatas;
		s[fib.st_size] = 255;
	}
	return __confdatas;
}

struct DayDream_Conference *dd_getconf(int cnum)
{
	struct DayDream_Conference *tconf;
	struct DayDream_MsgBase *tbase;
	int bcnt;

	if (!(tconf = dd_getconfdata()))
		return 0;

	for (;;) {
		if (tconf->CONF_NUMBER == 255) 
			return NULL;

		if (tconf->CONF_NUMBER == cnum) 
			return tconf;
		
		tbase = (struct DayDream_MsgBase *) tconf + 1;
		bcnt = tconf->CONF_MSGBASES;
		while (bcnt) {
			tbase++;
			bcnt--;
		}
		tconf = (struct DayDream_Conference *) tbase;
	}

}

struct DayDream_MsgBase *dd_getbase(int cnum, int bnum)
{
	struct DayDream_Conference *tconf;
	int bcnt;
	struct DayDream_MsgBase *tbase;

	tconf = dd_getconf(cnum);
	if (!tconf)
		return NULL;

	tbase = (struct DayDream_MsgBase *) tconf + 1;
	bcnt = tconf->CONF_MSGBASES;
	while (bcnt) {
		if (tbase->MSGBASE_NUMBER == bnum)
			return tbase;
		tbase++;
		bcnt--;
	}
	return NULL;
}

void dd_sendstring(struct dif *d, const char *str)
{
	while (strlen(str) > sizeof(d->dif_dm.ddm_string) - 1) {
		d->dif_dm.ddm_command = 2;
		strlcpy(d->dif_dm.ddm_string, str,
			sizeof(d->dif_dm.ddm_string));
		writedm(d);
		str = &str[sizeof(d->dif_dm.ddm_string) - 1];
	}
	d->dif_dm.ddm_command = 2;
	strlcpy(d->dif_dm.ddm_string, str, sizeof(d->dif_dm.ddm_string));
	writedm(d);
}

int dd_sendfmt(struct dif *d, const char *fmt, ...)
{
	char *buf;
	va_list args;
	int len;

	va_start(args, fmt);
	len = vasprintf(&buf, fmt, args);
	va_end(args);

	if (len == -1)
		return -1;

	dd_sendstring(d, buf);
	free(buf);

	return len;
}

int dd_prompt(struct dif *d, char *buffer, int len, int flags)
{
	strlcpy(d->dif_dm.ddm_string, buffer, sizeof d->dif_dm.ddm_string);
	d->dif_dm.ddm_data1 = len;
	d->dif_dm.ddm_data2 = flags;
	d->dif_dm.ddm_command = 3;
	writedm(d);
	strlcpy(buffer, d->dif_dm.ddm_string, len);
	return d->dif_dm.ddm_data1;
}

int dd_hotkey(struct dif *d, int flags)
{
	d->dif_dm.ddm_command = 4;
	d->dif_dm.ddm_data1 = flags;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_typefile(struct dif *d, const char *text, int flags)
{
	d->dif_dm.ddm_command = 5;
	d->dif_dm.ddm_data1 = flags;
	strlcpy(d->dif_dm.ddm_string, text, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data3;
}

int dd_flagsingle(struct dif *d, char *file, int flags)
{
	d->dif_dm.ddm_command = 6;
	d->dif_dm.ddm_data1 = flags;
	strlcpy(d->dif_dm.ddm_string, file, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_findusername(struct dif *d, char *user)
{
	d->dif_dm.ddm_command = 7;
	strlcpy(d->dif_dm.ddm_string, user, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_system(struct dif *d, char *user, int mode)
{
	d->dif_dm.ddm_command = 8;
	d->dif_dm.ddm_data1 = mode;
	strlcpy(d->dif_dm.ddm_string, user, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

int dd_docmd(struct dif *d, char *cmd)
{
	d->dif_dm.ddm_command = 9;
	strlcpy(d->dif_dm.ddm_string, cmd, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

int dd_writelog(struct dif *d, char *cmd)
{
	d->dif_dm.ddm_command = 10;
	strlcpy(d->dif_dm.ddm_string, cmd, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

int dd_changestatus(struct dif *d, char *stat)
{
	d->dif_dm.ddm_command = 11;
	strlcpy(d->dif_dm.ddm_string, stat, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return 1;
}

void dd_pause(struct dif *d)
{
	d->dif_dm.ddm_command = 12;
	writedm(d);
}

int dd_joinconf(struct dif *d, int dc, int fl)
{
	d->dif_dm.ddm_command = 13;
	d->dif_dm.ddm_data1 = dc;
	d->dif_dm.ddm_data2 = fl;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isfreedl(struct dif *d, char *s)
{
	d->dif_dm.ddm_command = 14;
	strlcpy(d->dif_dm.ddm_string, s, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_flagfile(struct dif *d, char *s, int i)
{
	d->dif_dm.ddm_command = 15;
	d->dif_dm.ddm_data1 = i;
	strlcpy(d->dif_dm.ddm_string, s, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_getstrval(struct dif *d, char *buf, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	strcpy(buf, d->dif_dm.ddm_string);
}

void dd_getstrlval(struct dif *d, char *buf, size_t len, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	strlcpy(buf, d->dif_dm.ddm_string, len);
}

void dd_setstrval(struct dif *d, char *buf, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 1;
	strlcpy(d->dif_dm.ddm_string, buf, sizeof d->dif_dm.ddm_string);
	writedm(d);
}

int dd_getintval(struct dif *d, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_setintval(struct dif *d, int val, int new)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 1;
	d->dif_dm.ddm_data2 = new;
	writedm(d);
}

uint64_t dd_getlintval(struct dif *d, int val)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 0;
	writedm(d);
	return d->dif_dm.ddm_ldata;
}

void dd_setlintval(struct dif *d, int val, uint64_t new)
{
	d->dif_dm.ddm_command = val;
	d->dif_dm.ddm_data1 = 1;
	d->dif_dm.ddm_ldata = new;
	writedm(d);
}

void dd_getlprs(struct dif *d, struct DayDream_LRP *lp)
{
	d->dif_dm.ddm_command = 16;
	writedm(d);
	lp->lrp_read = d->dif_dm.ddm_data1;
	lp->lrp_scan = d->dif_dm.ddm_data2;
}

void dd_setlprs(struct dif *d, struct DayDream_LRP *lp)
{
	d->dif_dm.ddm_command = 17;
	d->dif_dm.ddm_data1 = lp->lrp_read;
	d->dif_dm.ddm_data2 = lp->lrp_scan;
	writedm(d);
}

void writedm(struct dif *d)
{
	write(d->dif_sockfd, &d->dif_dm, sizeof(struct DayDream_DoorMsg));
	read(d->dif_sockfd, &d->dif_dm, sizeof(struct DayDream_DoorMsg));
}

int dd_isconfaccess(struct dif *d, int confn)
{
	d->dif_dm.ddm_command = 18;
	d->dif_dm.ddm_data1 = confn;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isanybasestagged(struct dif *d, int confa)
{
	d->dif_dm.ddm_command = 19;
	d->dif_dm.ddm_data1 = confa;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isconftagged(struct dif *d, int n)
{
	d->dif_dm.ddm_command = 20;
	d->dif_dm.ddm_data1 = n;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_isbasetagged(struct dif *d, int n, int b)
{
	d->dif_dm.ddm_command = 21;
	d->dif_dm.ddm_data1 = n;
	d->dif_dm.ddm_data2 = b;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_getmprs(struct dif *d, struct DayDream_MsgPointers *lp)
{
	d->dif_dm.ddm_command = 22;
	writedm(d);
	lp->msp_high = d->dif_dm.ddm_data1;
	lp->msp_low = d->dif_dm.ddm_data2;
}

void dd_setmprs(struct dif *d, struct DayDream_MsgPointers *lp)
{
	d->dif_dm.ddm_command = 23;
	d->dif_dm.ddm_data1 = lp->msp_high;
	d->dif_dm.ddm_data2 = lp->msp_low;
	writedm(d);
}

int dd_changemsgbase(struct dif *d, int base, int flags)
{
	d->dif_dm.ddm_command = 24;
	d->dif_dm.ddm_data1 = base;
	d->dif_dm.ddm_data2 = flags;
	writedm(d);
	return d->dif_dm.ddm_data1;
}

void dd_sendfiles(struct dif *d, char *flist)
{
	d->dif_dm.ddm_command = 25;
	strlcpy(d->dif_dm.ddm_string, flist, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return;
}

void dd_getfiles(struct dif *d, char *path)
{
	d->dif_dm.ddm_command = 26;
	strlcpy(d->dif_dm.ddm_string, path, sizeof d->dif_dm.ddm_string);
	writedm(d);
}

/* dd_fileattach is a private command :) */

int dd_fileattach(struct dif *d)
{
	d->dif_dm.ddm_command = 27;
	writedm(d);
	return 1;
}

int dd_unflagfile(struct dif *d, char *s)
{
	d->dif_dm.ddm_command = 28;
	strlcpy(d->dif_dm.ddm_string, s, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_findfilestolist(struct dif *d, char *file, char *list)
{
	if (strlen(file) > 48)
		return 0;

	d->dif_dm.ddm_command = 29;
	strlcpy(d->dif_dm.ddm_string, file, 48);
	strlcpy(&d->dif_dm.ddm_string[50], list,
		sizeof(d->dif_dm.ddm_string) - 50);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_getfidounique(void)
{
	int fn;
	char buf[1024];
	int i = 0;
	char *home;

	if ((home = getenv("DAYDREAM")) == NULL)
		return 0;
	snprintf(buf, sizeof buf, "%s/data/fidocnt.dat", home);
	fn = open(buf, O_CREAT | O_RDWR, 0644);
	if (fn < 0)
		return 0;
	read(fn, &i, sizeof(int));
	i++;
	lseek(fn, 0, SEEK_SET);
	write(fn, &i, sizeof(int));
	close(fn);
	return i;
}

int dd_isfiletagged(struct dif *d, char *str)
{
	d->dif_dm.ddm_command = 30;
	strlcpy(d->dif_dm.ddm_string, str, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}

int dd_dumpfilestofile(struct dif *d, char *str)
{
	d->dif_dm.ddm_command = 31;
	strlcpy(d->dif_dm.ddm_string, str, sizeof d->dif_dm.ddm_string);
	writedm(d);
	return d->dif_dm.ddm_data1;
}
