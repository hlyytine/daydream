#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <data.h>
#include <fcntl.h>

int readconfig(void)
{
	char buf[1024];
	struct stat st;
	int cfgfd;
	int i;
	char *s;

	nlines=5;
	isize=5;
	ilines=10;
	
	sprintf(buf,"%s/configs/server.cfg",getenv("DAYDREAM"));

	if (stat(buf,&st)==-1) {
		printf("Can't load config! ($DAYDREAM/configs/server.cfg)\n");
		return 0;
	}
	cfg=(char *)malloc(st.st_size+2);
		
	cfgfd=open(buf,O_RDONLY);
	i=read(cfgfd,cfg,st.st_size);
	close(cfgfd);
	cfg[i]=0;

	s=ecfg(cfg,"NODELINES ");
	if (s) nlines=atoi(s);

	s=ecfg(cfg,"INFOBOX ");
	if (s) isize=atoi(s);

	s=ecfg(cfg,"INFOLINES ");
	if (s) ilines=atoi(s);

	s=ecfg(cfg,"NODEVIEWER \"");
	if (s) {
		CpyToLAINA(s,viewcmd);
	} else {
		*viewcmd=0;
	}
	lcmem=(char *)malloc(ilines*70);
	lclines=(char **)malloc(ilines*sizeof(char *));

	lumem=(char *)malloc(ilines*70);
	lulines=(char **)malloc(ilines*sizeof(char *));
	
	for (i=0;i<ilines;i++) {
		lulines[i]=&lumem[i*68];
	}

	for (i=0;i<ilines;i++) {
		lclines[i]=&lcmem[i*68];
	}
	sprintf(buf,"%s/logfiles/callerslog.dat",getenv("DAYDREAM"));
	clogfd=open(buf,O_RDONLY);
	if (clogfd < 0) {
		printf("Can't load callerslog. Probably too old DD version. Get 2.02 or later.\n");
		return 0;
	}
	sprintf(buf,"%s/logfiles/uploadlog.dat",getenv("DAYDREAM"));
	ulogfd=open(buf,O_RDONLY);
	if (ulogfd < 0) {
		printf("Can't load UPLOADLOG. Make somebody upload something!\n");
		return 0;
	}
	makelulist();
	fstat(ulogfd,&st);
	oldlusize=st.st_size;
	makelclist();
	fstat(clogfd,&st);
	oldlcsize=st.st_size;
	return 1;
}

char *ecfg(char *hay, char *need)
{
	char *s;
	while(1)
	{
		s=need;
		if (*hay==0) return 0;
		if (*hay==';') { 
			while(*hay!=10) {
				if(*hay==0) return 0;
				hay++;
			}
			continue;
		}
		while (1) {
			if (*s++==*hay++) {
				if (*s==0) return hay;
			} else {
				break;
			}
		}
	}
}

void CpyToLAINA(char *src, char *dest)
{
int i=0;

	while (src[i]!='\"') {
		dest[i]=src[i];
		i++;
	}
	dest[i]=0;
}

pid_t execute(char *s, int node)
{
	char *args[256];
	char buf[1024];
	char *t;
	int i=0;
	pid_t kid;
		
	sprintf(buf,s,node,node,node,node);
	t=buf;
	
	while (*t)
	{
		if (*t=='\'') {
			t++;
			args[i++]=t;
			while(*t!='\'') t++;
			*t++=0;
			while(*t==' ') t++;
			continue;
		}
		args[i++]=t;
		while (*t!=' ' && *t) t++;
		if (*t==' ') *t++=0;
	}
	args[i]=0;
	kid=fork();
	switch(kid)
	{
		case 0:
			execvp(args[0],&args[0]);
			exit(0);
		default:
			return kid;			
		case -1:
			return -1;
	}
}

