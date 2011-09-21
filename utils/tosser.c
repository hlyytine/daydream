#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>

#include <ddcommon.h>
#include <dd.h>
#include <ddlib.h>

enum {
	MAILTYPE_ECHO = 0x01,
	MAILTYPE_NET  = 0x02
};

static char *cfg;
static struct DayDream_MainConfig maincfg;

static struct DayDream_Conference *confs;
static struct DayDream_Conference *tc;
static struct DayDream_MsgBase *tb;

static const char *months[] = {
      "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

struct onodeinfo {
	struct Node nhead;
	unsigned short	NI_Zone;
	unsigned short	NI_Net;
	unsigned short	NI_Node;
	unsigned short	NI_Point;
	unsigned short	NI_OZone;
	unsigned short	NI_ONet;
	unsigned short	NI_ONode;
	unsigned short	NI_OPoint;
	unsigned short  NI_Akak;
	int		NI_FHandle;
	int		NI_FlowHandle;
	char		NI_Packer;
	char		NI_Type;
	unsigned short	NI_Msgs;
	unsigned short	NI_Sendflag;
	unsigned short  NI_Conv;
	char		NI_Pktname[1024];
	char		NI_Arcname[256];
	unsigned char	NI_ConvTable[260];
};

struct aka {
	int zone;
	int net;
	int node;
	int point;
};

struct basei {
	int confn;
	int basen;
};

static unsigned char ctable[256];
static int usetab=0;

static struct List *outlist;

struct ftsheader {
	unsigned short origNode;
	unsigned short destNode;
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
	unsigned short baud;
	unsigned short id;
	unsigned short origNet;
	unsigned short destNet;
	unsigned short prodcode;
	char password[8];
	unsigned short origZoneq;
	unsigned short destZoneq;
	unsigned short AuxNet;
	unsigned short cw2;
	unsigned short prodcode2;
	unsigned short cw;
	unsigned short origZone;
	unsigned short destZone;	
	unsigned short origPoint;
	unsigned short destPoint;
	char prdata[4];	
};

struct ftsmsg {
	unsigned short id;
	unsigned short origNode;
	unsigned short destNode;
	unsigned short origNet;
	unsigned short destNet;
	unsigned short Attribute;
	unsigned short Cost;
	char DateTime[20];
};


static char incoming[1024];
static char outgoing[1024];
static char unpdir[1024];
static char olddir[1024];
static char unpcmd[1024];

static struct basei bad;
static struct basei netm;
static int highest, lowest;

static struct List *NewList(void);
static void die(void);
static int parsecfg(void);
static char *ExamineCfg(char *, const char *);
static time_t ftndatetodd(char *);
static char *strtoaka(char *s, struct aka *ak);
static struct onodeinfo *findni(struct aka *ak);
static int export(void);
static void inbound(void);
static void closeout(void);
static int procnmsg(struct DayDream_Message *);
static int procemsg(struct DayDream_Message *);
static int wildcmp (char *, char *);
static void ftndate(time_t, char *);
static int writenetom(char *, struct onodeinfo *, 
	struct ftsmsg *, struct DayDream_Message *);
static void makeseenby(char *, char *);
static void cpytolaina(char *, char *);
static int checkarea(struct aka *, int, int);
static void handleipkt(char *);
static int procpkt(char *);
static int locatetag(char *);
static void getmsgptrs(void);
static int setmsgptrs(void);
static void makeftsh(struct onodeinfo *);
static void AddTail(struct List *, struct Node *);
static void convcpy(char *, char *, int);

int main(int argc, char *argv[])
{
	char buf[1024];
	int cfgfd;
	struct stat st;
	
	umask(~(0775));

	sprintf(buf,"%s/configs/tosser.cfg",getenv("DAYDREAM"));
	if (stat(buf,&st)==-1) {
		perror("Configfile error");
		exit(0);
	}
	cfg=(char *)malloc(st.st_size+2);
	memset(cfg,0,st.st_size+2);
	cfgfd = open(buf, O_RDONLY);
	if (cfgfd == -1) {
		perror("Configfile error");
		exit(0);
	}
	read(cfgfd,cfg,st.st_size);
	close(cfgfd);

	sprintf(buf,"%s/data/daydream.dat",getenv("DAYDREAM"));
	cfgfd = open(buf, O_RDONLY);
	if (cfgfd == -1) {
		perror("Can't open DD datafiles");
		exit(0);
	}
	read(cfgfd,&maincfg,sizeof(struct DayDream_MainConfig));
	close(cfgfd);

	setgid(maincfg.CFG_BBSGID);
	setegid(maincfg.CFG_BBSGID);
	setuid(maincfg.CFG_BBSUID);
	seteuid(maincfg.CFG_BBSUID);
	

	confs=dd_getconfdata();
	tmpnam(unpdir);
	mkdir(unpdir,0775);
	getcwd(olddir,1024);
	chdir(unpdir);

	atexit(die);
	if (!parsecfg()) {
		printf("Error in configfile!\n");
		exit(0);
	}
	export();	
	inbound();
	export();
	closeout();
	return 0;
}

static void die(void)
{
	chdir(olddir);
	deldir(unpdir);
	rmdir(unpdir);
}

static int process_mail(const char *filename, int type)
{
	struct DayDream_Message msg;
	int fd, retvalue;
	off_t ptr;

	fd = open(filename, O_RDWR);
	if (fd == -1)
		return -1;
	ptr = 0;
	while (pread(fd, &msg, sizeof(struct DayDream_Message), ptr) ==
		sizeof(struct DayDream_Message)) {
		if ((msg.MSG_FLAGS & (1L << 2)) == 0)
			goto next_message;

		if (type == MAILTYPE_ECHO)
			retvalue = procemsg(&msg);
		else if (type == MAILTYPE_NET)
			retvalue = procnmsg(&msg);
		else 
			abort();	
			
		if (!retvalue) {
			fputs("warning: cannot process echomail.\n", stderr);
			goto next_message;
		}
		msg.MSG_FLAGS &= ~(1L << 2);
		if (pwrite(fd, &msg, sizeof(struct DayDream_Message), ptr) !=
			sizeof(struct DayDream_Message)) {
			fputs("error: short write, cannot process mail.\n", stderr);
			break;
		}
next_message:
		ptr += sizeof(struct DayDream_Message);		
	}
	close(fd);
	return 0;
}

static int export(void)
{
	confs=(struct DayDream_Conference *)dd_getconfdata();
	if (!confs) return 0;
	tc=confs;

	while(1) {
		char mailbuf[1024];
		int bcnt;
		
		if (tc->CONF_NUMBER == 255) 
			break;

		/* FIXME: Very suspicious code */
		tb = (struct DayDream_MsgBase *) tc + 1;
		for(bcnt = tc->CONF_MSGBASES; bcnt; bcnt--, tb++) {
			sprintf(mailbuf, "%s/messages/base%3.3d/msgbase.dat",
				tc->CONF_PATH, tb->MSGBASE_NUMBER);
			
			if (toupper(tb->MSGBASE_FN_FLAGS) == 'E') {
				/* Fido tag required */
				if (!*tb->MSGBASE_FN_TAG) 
					continue;
				process_mail(mailbuf, MAILTYPE_ECHO);
			} else if (toupper(tb->MSGBASE_FN_FLAGS) == 'N') 
				process_mail(mailbuf, MAILTYPE_NET);
		}
		/* FIXME: Very suspicious code */
		tc = (struct DayDream_Conference *) tb;
	}
	return 1;
}

static int routeadd(struct aka *sak, struct aka *dak)
{
	char buf1[512];
	char buf2[512];
	char *s;
	char *t;
	
	sprintf(buf1,"%d:%d/%d.%d",sak->zone,sak->net,sak->node,sak->point);
	s=ExamineCfg(cfg,"\nROUTE:\n");
	while(*s!='~'&&*s) {
		t=buf2;
		while(*s!=' ') *t++=*s++;
		*t=0;
		if (wildcmp(buf1,buf2)) {
			strtoaka(s,dak);
			return 1;
		} else {
			while(*s!=10 && *s) s++;
			s++;
		}
	}
	memcpy(dak,sak,sizeof(struct aka));
	return 1;
}

static int wildcmp (char *nam, char *pat)
{
	char *p;            

	for (;;)
	{
		if (tolower(*nam) == tolower(*pat)) {
			if(*nam++ == '\0')  return(1);
			pat++;
		} else if (*pat == '?' && *nam != 0) {
		    	nam++;
		    	pat++;
		} else	break;
	}

	if (*pat != '*') return(0);

	while (*pat == '*') {
		if (*++pat == '\0')  return(1);
	}

	for (p=nam+strlen(nam)-1;p>=nam;p--) {
		if (tolower(*p) == tolower(*pat))
			if (wildcmp(p,pat) == 1) return(1);
	}
	return 0;
}

static int procnmsg(struct DayDream_Message *msg)
{
	struct ftsmsg me;
	struct aka ak;
	struct aka dak;
	
	char buf[1024];
	char msgn[1024];
	char *s;
	struct stat st;
	int i;
	struct onodeinfo *on;
	if (!(msg->MSG_FLAGS & (1L<<2))) return 1;
	
	sprintf(msgn,"%s/messages/base%3.3d/msg%5.5d",tc->CONF_PATH,tb->MSGBASE_NUMBER,msg->MSG_NUMBER);
	if (stat(msgn,&st)==-1) return 0;
	
	memset(&me,0,sizeof(struct ftsmsg));
	me.id=0x0002;
	me.origNode=msg->MSG_FN_ORIG_NODE;
	me.destNode=msg->MSG_FN_DEST_NODE;
	me.origNet=msg->MSG_FN_ORIG_NET;
	me.destNet=msg->MSG_FN_DEST_NET;
	if (msg->MSG_FLAGS & (1L<<0)) me.Attribute |= (1L<<0);
	ftndate(msg->MSG_CREATION,me.DateTime);
	
	/* Check if exporting to home :) */
	for(i=1;;i++) {
		sprintf(buf,"\nAKA%d ",i);
		s=ExamineCfg(cfg,buf);
		if (!s) break;

		strtoaka(s,&ak);
		if ( (ak.zone == msg->MSG_FN_DEST_ZONE) &&
		    (ak.net == msg->MSG_FN_DEST_NET) &&
		    (ak.node == msg->MSG_FN_DEST_NODE) &&
		    (ak.point == msg->MSG_FN_DEST_POINT)) return 1;
	}
	
	on=(struct onodeinfo *)outlist->lh_Head;
	while(on->nhead.ln_Succ)
	{
		if ( (on->NI_Zone == msg->MSG_FN_DEST_ZONE) &&
		    (on->NI_Net == msg->MSG_FN_DEST_NET) &&
		    (on->NI_Node == msg->MSG_FN_DEST_NODE) && 
		    (on->NI_Point == msg->MSG_FN_DEST_POINT)) {
			writenetom(msgn,on,&me,msg);
		}
		on=(struct onodeinfo *)on->nhead.ln_Succ;
	}
	ak.zone=msg->MSG_FN_DEST_ZONE;
	ak.net=msg->MSG_FN_DEST_NET;
	ak.node=msg->MSG_FN_DEST_NODE;
	ak.point=msg->MSG_FN_DEST_POINT;
	routeadd(&ak,&dak);

	on=(struct onodeinfo *)outlist->lh_Head;
	while(on->nhead.ln_Succ)
	{
		if ( (on->NI_Zone == dak.zone) &&
		    (on->NI_Net == dak.net) &&
		    (on->NI_Node == dak.node) && 
		    (on->NI_Point == dak.point)) {
			writenetom(msgn,on,&me,msg);
		}
		on=(struct onodeinfo *)on->nhead.ln_Succ;
	}
	return 1;
}

static int writenetom(char *msgname, struct onodeinfo *on, struct ftsmsg *me, struct DayDream_Message *msg) 
{
	
	FILE *of;
	FILE *inf;
	unsigned char c;
	int gna;
	
	inf=fopen(msgname,"r");
	if (!inf) return 0;
	of=fopen(on->NI_Pktname,"a");
	if (!of) return 0;
	fwrite(me,sizeof(struct ftsmsg),1,of);
	if (*msg->MSG_RECEIVER==0 || *msg->MSG_RECEIVER==-1) {
		fputs("All",of);
		fputc(0,of);
	} else {
		fputs(msg->MSG_RECEIVER,of);
		fputc(0,of);
	}
	fputs(msg->MSG_AUTHOR,of);
	fputc(0,of);
	fputs(msg->MSG_SUBJECT,of);
	fputc(0,of);
	
	while ((gna=fgetc(inf)))
	{
		c=gna;
		if (!c) break;
		if (gna==EOF) break;
		if (c==13) continue;
		if (c==10) {
			fputc(13,of);
			continue;
		}
		if (on->NI_Conv) {
			fputc(on->NI_ConvTable[c],of);
		} else {
			fputc(c,of);
		}
	}
	fputc(0,of);
	fclose(of);
	on->NI_Msgs++;
	return 1;
}

static int procemsg(struct DayDream_Message *msg)
{
	struct ftsmsg me;
	struct aka ak;
	char buf[1024];
	char *s;
	char *mb;
	struct stat st;
	int fdii;
	
	if (!(msg->MSG_FLAGS & (1L<<2))) return 0;

	sprintf(buf,"%s/messages/base%3.3d/msg%5.5d",tc->CONF_PATH,tb->MSGBASE_NUMBER,msg->MSG_NUMBER);
	if (stat(buf,&st)==-1) {
		perror(buf);
		return 0;
	}
	fdii=open(buf,O_RDONLY);
	if (fdii < 0) return 0;
	
	mb=(char *)malloc(st.st_size+2);
	memset(mb,0,st.st_size+2);
	read(fdii,mb,st.st_size);
	close(fdii);
	
	memset(&me,0,sizeof(struct ftsmsg));
	me.id=0x0002;
	me.origNode=msg->MSG_FN_ORIG_NODE;
	me.destNode=msg->MSG_FN_DEST_NODE;
	me.origNet=msg->MSG_FN_ORIG_NET;
	me.destNet=msg->MSG_FN_DEST_NET;
	if (msg->MSG_FLAGS & (1L<<0)) me.Attribute |= (1L<<0);
	ftndate(msg->MSG_CREATION,me.DateTime);

	sprintf(buf,"\nAREA %d:%d ",tc->CONF_NUMBER,tb->MSGBASE_NUMBER);
	s=ExamineCfg(cfg,buf);
	if (s) {
		struct onodeinfo *on;
		char sbmem[8192];
		char pathmem[4096];
		unsigned char *se;
		unsigned char *sr=0;
		
		if ((se=strstr(mb,"\nSEEN-BY: "))) {
			*se=0;
			se+=9;
			makeseenby(se,sbmem);
			sr=se;
		} else {
			makeseenby(0,sbmem);
		}

		if (sr) {
			sr=strstr(sr,"\001PATH: ");
		}
		if (!sr) {
			sprintf(pathmem,"\001PATH: %d/%d\r",tb->MSGBASE_FN_NET,tb->MSGBASE_FN_NODE);
		} else {
			char *mur;
			char *f, *g;
			char smb[40];
			int net;
			f=sr;

			while((mur=strstr(sr+2,"\001PATH: "))) sr=mur;
			while(*sr && *sr!=13 && *sr!=10) sr++;

			sr--;
			while(*sr!='/') sr--;
			while(*sr!=' ') sr--;
			sr++;
			net=atoi(sr);
			while(*sr && *sr!=13 && *sr!=10) sr++;
			*sr=0;
			g=f;
			while(*++g) if(*g==10) *g=13;
			strcpy(pathmem,f);
			if (net!=tb->MSGBASE_FN_NET) {
				sprintf(smb," %d/%d\r",tb->MSGBASE_FN_NET,tb->MSGBASE_FN_NODE);
			} else {
				sprintf(smb," %d\r",tb->MSGBASE_FN_NODE);
			}
			strcat(pathmem,smb);
		}

		while((s=strtoaka(s,&ak)))
		{
			
			if (msg->MSG_FN_PACKET_ORIG_NET==ak.net &&
			    msg->MSG_FN_PACKET_ORIG_NODE==ak.node &&
			    msg->MSG_FN_PACKET_ORIG_POINT==ak.point) continue;
			    
			if ((on=findni(&ak))) {
				
				FILE *of;
				me.destNode=on->NI_Node;
				me.destNet=on->NI_Net;
				
				of=fopen(on->NI_Pktname,"a");
				if (!of) continue;
				fwrite(&me,sizeof(struct ftsmsg),1,of);
				if (*msg->MSG_RECEIVER==0 || *msg->MSG_RECEIVER==-1) {
					fputs("All",of);
					fputc(0,of);
				} else {
					fputs(msg->MSG_RECEIVER,of);
					fputc(0,of);
				}
				fputs(msg->MSG_AUTHOR,of);
				fputc(0,of);
				fputs(msg->MSG_SUBJECT,of);
				fputc(0,of);

				se=mb;
				for (;*se;se++)
				{
					if (!*se) break;
					if (*se==13) continue;
					if (*se==10) {
						fputc(13,of);
						continue;
					}
					if (on->NI_Conv) {
						fputc(on->NI_ConvTable[*se],of);
					} else {
						fputc(*se,of);
					}
				}
				fputs(sbmem,of);
				fputs(pathmem,of);
				fputc(0,of);
				fclose(of);
				on->NI_Msgs++;
			}
		}
	}
	free(mb);
	
	
	return 1;
}

static void closeout(void)
{
	struct onodeinfo *myo;
	const char *days[] = { "su", "mo", "tu", "we", "th", "fr", "sa" };
	
	myo=(struct onodeinfo *)outlist->lh_Head;
	while(myo->nhead.ln_Succ)
	{

		if (myo->NI_Msgs) {
			FILE *fi;
			struct tm *tm;
			time_t tim;
			char humn[80];
			
			char fl;
			char floname[1024];
			char finpkt[1024];
			char pakcmd[2048];
			char *s;
			char pala[512];
			char og[512];
			
			fi=fopen(myo->NI_Pktname,"a");
			if (!fi) {
				perror(myo->NI_Pktname);
			}
			fputc(0,fi);
			fputc(0,fi);
			fclose(fi);
			switch(myo->NI_Type)
			{
				case 'N':
					fl='f';
					break;
				case 'C':
					fl='c';
					break;
				case 'D':
					fl='d';
					break;
				case 'H':
					fl='h';
					break;
			}
			if (myo->NI_Akak==1) {
				sprintf(og,"%s/",outgoing);
			} else {
				sprintf(og,"%s.%03x/",outgoing,myo->NI_Zone);
			}
			if (!myo->NI_Point) {
				sprintf(floname,"%s%04x%04x.%clo",og,myo->NI_Net,myo->NI_Node,fl);
			} else {
				sprintf(floname,"%s%04x%04x.pnt/%08x.%clo",og,myo->NI_Net,myo->NI_Node,myo->NI_Point,fl);
			}
			tim=time(0);
			tm=localtime(&tim);
			if (!myo->NI_Point) {
				sprintf(finpkt,"^%s%04x%04x.%s%d",
					og,myo->NI_ONet,
					myo->NI_ONode,days[tm->tm_wday],(tm->tm_yday/7)%9);
			} else {
				sprintf(finpkt,"^%s%04x%04x.pnt/%08x.%s%d",
					og,myo->NI_Net,
					myo->NI_Node,myo->NI_Point,days[tm->tm_wday],(tm->tm_yday/7)%9);
			}
			sprintf(humn,"\nPACK%d \"",myo->NI_Packer);
			s=ExamineCfg(cfg,humn);
			if (s) {
				FILE *korva;
				char korvabuf[1024];
				int joo=1;
				cpytolaina(pala,s);
				
				sprintf(pakcmd,pala,&finpkt[1],myo->NI_Pktname);
				system(pakcmd);
				korva=fopen(floname,"w+");
				if (korva) {
					while(fgets(korvabuf,1024,korva))
					{
						if (!strcasecmp(korvabuf,finpkt)) {
							joo=0;
							break;
						}
					}
					if (joo) fprintf(korva,"%s\n",finpkt);
					fclose(korva);
				}
			}
		} else {
			unlink(myo->NI_Pktname);
		}
		myo=(struct onodeinfo *)myo->nhead.ln_Succ;		
	}
}

static void makeseenby(char *s, char *buf)
{
	unsigned short ams[1024];
	unsigned short ams2[1024];
	unsigned short *i, *l;
	struct aka ak;
	struct onodeinfo *myo;
	unsigned short sma=0;
		
	i=ams;
	
	if (s) {
		while (*s)
		{
			unsigned short j;
			unsigned short k;

			while(*s==' ') s++;
			j=atoi(s);

			while(isdigit(*s)) s++;
			if (*s=='/') {
				if (j) {
					k=j;
				}
				s++;
			} else if (*s==13 || *s==10) {
				if (j && k) {
					*i++=k;
					*i++=j;
				}
				s=strstr(s,"SEEN-BY: ");
				if (!s) break; 
			} else {
				if (j && k) {
					*i++=k;
					*i++=j;
				}
				s++;
			}				
		}
	}
	
	*i=65535;
	myo=(struct onodeinfo *)outlist->lh_Head;
	while(myo->nhead.ln_Succ)
	{
		ak.zone=myo->NI_Zone;
		ak.node=myo->NI_Node;
		ak.net=myo->NI_Net;
		ak.point=myo->NI_Point;
		if (checkarea(&ak,tc->CONF_NUMBER,tb->MSGBASE_NUMBER)) {
			if (!myo->NI_Point) {
				unsigned short *wu;
				int ok=1;
				wu=ams;
				while(*wu!=65535) {
					if (*wu==myo->NI_Net && *(wu+1)==myo->NI_Node) {
						ok=0;
						break;
					}
					wu+=2;
				}
				if (ok) {
					*i++=myo->NI_Net;
					*i++=myo->NI_Node;
					*i=65535;
				}
			}
		}
		myo=(struct onodeinfo *)myo->nhead.ln_Succ;		
	}

	
	i=ams;
	l=ams2;
	while (sma!=65535)
	{
		sma=65535;
		i=ams;
		while(*i!=65535)
		{
			if (*i && (*i < sma)) sma=*i;
			i+=2;
		}
		i=ams;

		if (sma==65535) break;
		while(*i!=65535)
		{
			if (*i==sma) {
				*l++=*i;
				*i++=0;
				*l++=*i;
				*i++=0;
			} else {
				i+=2;
			}
		}
	}
	*l=65535;


	l=ams;

	while(1)
	{
		unsigned short r;
		unsigned short *ba;

		i=ams2;
		
		while(*i==0) i+=2;
		if (*i==65535) break;
		r=*i;
		sma=65535;
		ba=i;
		
		while(*i==r) {
			i++;
			if (*i && (*i < sma)) sma=*i;
			i++;
		}
		if (sma==65535) {
			i=ba;
			while(*i==r) {
				*i++=0;
				i++;
			}
			continue;
		} 
		i=ba;
		while(*(i+1)!=sma) i+=2;
		*l++=*i++;
		*l++=*i;
		*i++=0;
	}
	*l++=65535;

	strcpy(buf,"\rSEEN-BY:");

	i=ams;
	sma=0;

	while(*i!=65535) {
		char sba[30];
		if (sma!=*i) {
			sma=*i;
			sprintf(sba," %d/%d",*i,*(i+1));
			i+=2;
			strcat(buf,sba);
		} else {
			i++;
			sprintf(sba," %d",*i++);
			strcat(buf,sba);
		}
	}
	strcat(buf,"\r");
}

static void inbound(void)
{
	struct dirent *de;
	DIR *dh;
	char ibuf[1024];
	
	if ((dh=opendir(incoming)))
	{
		while((de=readdir(dh)))
		{
			if ((strlen(de->d_name) == 12) &&
		               ((strncasecmp(de->d_name+8,".su",3) == 0) ||
		               (strncasecmp(de->d_name+8,".mo",3) == 0) ||
		               (strncasecmp(de->d_name+8,".tu",3) == 0) ||
		               (strncasecmp(de->d_name+8,".we",3) == 0) ||
		               (strncasecmp(de->d_name+8,".th",3) == 0) ||
		               (strncasecmp(de->d_name+8,".fr",3) == 0) ||
		               (strncasecmp(de->d_name+8,".sa",3) == 0))) {

				sprintf(ibuf,"%s%s",incoming,de->d_name);
				handleipkt(ibuf);
				unlink(ibuf);
			}			                                                                                             
		
		}
		closedir(dh);
	}
}

static struct onodeinfo *findni(struct aka *ak)
{
	struct onodeinfo *myo;
	
	myo=(struct onodeinfo *)outlist->lh_Head;
	while(myo->nhead.ln_Succ)
	{
		if (myo->NI_Zone==ak->zone &&
		    myo->NI_Net==ak->net &&
		    myo->NI_Node==ak->node &&
		    myo->NI_Point==ak->point) {
			return myo;    
		}
		myo=(struct onodeinfo *)myo->nhead.ln_Succ;		
	}
	return 0;
}

static int checkarea(struct aka *ak, int conf, int base)
{
	
	char buf[1024];
	char *s;
	struct aka ak2;
	sprintf(buf,"\nAREA %d:%d ",conf,base);
	s=ExamineCfg(cfg,buf);
	if (!s) return 0;

	while((s=strtoaka(s,&ak2)))
	{
		if (ak2.zone == ak->zone &&
		    ak2.net == ak->net &&
		    ak2.node == ak->node &&
		    ak2.point == ak->point) return 1;
	}
	return 0;	
}

static void handleipkt(char *pk)
{
	char pbuf[1024];
	struct dirent *de;
	DIR *dh;
	
	sprintf(pbuf,unpcmd,pk);
	system(pbuf);

	
	if ((dh=opendir(unpdir)))
	{
		while((de=readdir(dh)))
		{
			if ( (strlen(de->d_name)==12) && (strncasecmp(de->d_name+8,".pkt",4)==0) ) {
				procpkt(de->d_name);
				unlink(de->d_name);
			}
		}
		closedir(dh);
	}
}

static int procpkt(char *pn)
{
	int pfd;
	int seekpoint;
	unsigned char msgbuf[4098];
	int bread;
	struct ftsmsg *ms;
	struct DayDream_Message msg;
	unsigned char *s;
	int msgfd;
	char qbuf[1024];
	unsigned char *lastbyte;
	FILE *msgf;
	struct ftsheader he;
	char *t;
	
	lastbyte=&msgbuf[4095];
						
		
	pfd=open(pn,O_RDONLY);
	if (pfd < 0) return 0;
	read(pfd,&he,sizeof(struct ftsheader));
	seekpoint=sizeof(struct ftsheader);
	printf("Got a packet from %d:%d/%d.%d\n",he.origZone,he.origNet,he.origNode,he.origPoint);

	while(1)
	{
		lseek(pfd,seekpoint,SEEK_SET);
		memset(&msgbuf,0,4098);
		bread=read(pfd,&msgbuf,4096);
		if (!bread) break;
		ms=(struct ftsmsg *)&msgbuf;
		if (ms->id != 0x0002) break;

		memset(&msg,0,sizeof(struct DayDream_Message));

		msg.MSG_FN_PACKET_ORIG_ZONE=he.origZone;
  		msg.MSG_FN_PACKET_ORIG_NET=he.origNet;
  		msg.MSG_FN_PACKET_ORIG_NODE=he.origNode;
  		msg.MSG_FN_PACKET_ORIG_POINT=he.origPoint;

 		msg.MSG_FN_ORIG_NET=ms->origNet;
 		msg.MSG_FN_ORIG_NODE=ms->origNode;
 		
		msg.MSG_FN_DEST_NET=ms->destNet;
		msg.MSG_FN_DEST_NET=ms->destNode;
		
		if (ms->Attribute & (1L<<0)) {
			msg.MSG_FLAGS |= (1L<<0);
		}
		
		msg.MSG_CREATION=ftndatetodd(ms->DateTime);
		seekpoint+=sizeof(struct ftsmsg);
		s=&msgbuf[sizeof(struct ftsmsg)];

		convcpy(msg.MSG_RECEIVER,s,25);
		seekpoint+=strlen(s)+1;
		s=&s[strlen(s)+1];

		convcpy(msg.MSG_AUTHOR,s,25);
		seekpoint+=strlen(s)+1;
		s=&s[strlen(s)+1];

		convcpy(msg.MSG_SUBJECT,s,67);
		seekpoint+=strlen(s)+1;
		s=&s[strlen(s)+1];

		if (!strncmp("AREA:",s,5)) {
			if (!locatetag(&s[5])) break;
		} else {
			tc=dd_getconf(netm.confn);
			tb=dd_getbase(netm.confn,netm.basen);
			if (!tb || !tc) {
				printf("Netmail error!\n");
				break;
			}
		}
		if ( (t=strstr(s,"\001TOPT "))) {
			t+=6;
			msg.MSG_FN_DEST_POINT=atoi(t);
		}

		if ( (t=strstr(s,"\001FMPT "))) {
			t+=6;
			msg.MSG_FN_ORIG_POINT=atoi(t);
		}
		
		if ( (t=strstr(s,"\001INLT "))) {
			t+=6;
			msg.MSG_FN_DEST_ZONE=atoi(t);
			while(*t++!=' ');
			msg.MSG_FN_ORIG_ZONE=atoi(t);
		}
		
		

		printf("Writing to: %s/%s..\n",tc->CONF_NAME,tb->MSGBASE_NAME);
		getmsgptrs();
		highest++;
		msg.MSG_NUMBER=highest;
		setmsgptrs();
		msg.MSG_FLAGS |= (1L<<2);
		sprintf(qbuf,"%s/messages/base%3.3d/msgbase.dat",tc->CONF_PATH,tb->MSGBASE_NUMBER);

		if ((msgfd=open(qbuf,O_RDWR|O_CREAT,0664)) < 0) {
			printf("Fatal write error!\n");
			break;
		}
		lseek(msgfd,0,SEEK_END);
		write(msgfd,&msg,sizeof(struct DayDream_Message));
		close(msgfd);

		sprintf(qbuf,"%s/messages/base%3.3d/msg%5.5d",tc->CONF_PATH,tb->MSGBASE_NUMBER,highest);
		msgf=fopen(qbuf,"w");
		
		if (!msgf) {
			printf("FATAL write error!\n");
			break;
		}
		while(1)
		{
			if (s==lastbyte) {
				read(pfd,&msgbuf,4096);
				s=msgbuf;
			}
			if (!*s) {
				break;
			}
			if (*s==10) {
				s++; seekpoint++;
				continue;
			}
			if (*s==13) {
				*s=10;
			} else {
				if (usetab) *s=ctable[*s];
			}
			fputc(*s,msgf);
			s++; seekpoint++;
		}
		fclose(msgf);
		seekpoint++;
	}	
	close(pfd);
	return 1;

}

static void convcpy(char *dest, char *src, int n)
{
	if (usetab) {
		while(n && *src) *dest++=ctable[(unsigned char)*src++];
		*dest=0;
	} else {
		strncpy(dest,src,n);
	}
}

static void getmsgptrs(void) 
{
	int msgfd;
	struct DayDream_MsgPointers ptrs;
	
	char gmpbuf[1024];
	sprintf(gmpbuf,"%s/messages/base%3.3d/msgbase.ptr",tc->CONF_PATH,tb->MSGBASE_NUMBER);

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
	
	char gmpbuf[1024];
	sprintf(gmpbuf,"%s/messages/base%3.3d/msgbase.ptr",tc->CONF_PATH,tb->MSGBASE_NUMBER);

	if ((msgfd=open(gmpbuf,O_RDWR|O_CREAT,0664)) < 0) {
		return 0;
	}

	ptrs.msp_high=highest;
	ptrs.msp_low=lowest;
	write(msgfd,&ptrs,sizeof(struct DayDream_MsgPointers));
	close(msgfd);
	return 1;
}

static int locatetag(char *s)
{
	char bu[1024];
	char *t;
	struct DayDream_Conference *mconf;
	struct DayDream_MsgBase *mbase;
	
	t=bu;
	while(*s && *s!=10 && *s!=13) *t++=*s++;
	*t=0;
	
	mconf=confs;
	
	while(1)
	{
		int bcnt;
		if (mconf->CONF_NUMBER==255) break;
		
		/* FIXME: Very suspicious code */
		mbase = (struct DayDream_MsgBase *) mconf + 1;
		bcnt=mconf->CONF_MSGBASES;
		for(bcnt=mconf->CONF_MSGBASES;bcnt;bcnt--,mbase++)
		{
			if (!strcasecmp(bu,mbase->MSGBASE_FN_TAG)) {
				tc=mconf;
				tb=mbase;
				return 1;
			}
		}	
		/* FIXME: Very suspicious code */
		mconf = (struct DayDream_Conference *) mbase;
	}
	tc=dd_getconf(bad.confn);
	tb=dd_getbase(bad.confn,bad.basen);
	if (!tc || !tb) {
		printf("Area error! (%s)\n",bu);
		return 0;
	}
	return 1;
}

static void getbasei(char *s, struct basei *b)
{
	char bu[1024];
	char *t;
	
	t=bu;
	while(*s && *s!=':') *t++=*s++;
	*t=0;
	b->confn=atoi(bu);
	s++;
	
	t=bu;
	while(*s && *s!=10 && *s!=13) *t++=*s++;
	*t=0;
	b->basen=atoi(bu);
}

static time_t ftndatetodd(char *t)
{
	struct tm tm;
	char foob[256];
	char monthlist[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	char *s;
	
	memset(&tm,0,sizeof(struct tm));
	s=foob;
	
	while(*t && *t!=' ') *s++=*t++;
	*s=0;
	tm.tm_mday=atoi(foob);
	if (!*t) return 0;
	t++;
	
	s=monthlist;
	for(tm.tm_mon=0;tm.tm_mon < 13; tm.tm_mon++)
	{
		if (!strncmp(s,t,3)) break;
		s=&s[3];
	}
	t=&t[4];
	
	s=foob;
	while(*t && *t!=' ') *s++=*t++;
	*s=0;
	tm.tm_year=atoi(foob);	
	if (!*t) return 0;
	/* I'm not really sure whether this date string suffers
	 * from year 2000 problem, but this hack shouldn't break
	 * anything.
	 */
	while (isdigit(*t))
		t++;
	if (tm.tm_year < 70)
		tm.tm_year += 100;
	
	s=foob;
	while(*t && *t!=':') *s++=*t++;
	*s=0;
	tm.tm_hour=atoi(foob);
	if (!*t) return 0;
	t++;

	s=foob;
	while(*t && *t!=':') *s++=*t++;
	*s=0;
	tm.tm_min=atoi(foob);
	if (!*t) return 0;
	t++;

	s=foob;
	while(*t) *s++=*t++;
	*s=0;
	tm.tm_sec=atoi(foob);

	return mktime(&tm);
}

static int parsecfg(void)
{
	char *s;
	struct basei bad;
	int i;
	char buf[1024];
	
	s=ExamineCfg(cfg,"INBOUND \"");
	if (!s) return 0;
	cpytolaina(incoming,s);

	s=ExamineCfg(cfg,"OUTBOUND \"");
	if (!s) return 0;
	cpytolaina(outgoing,s);

	s=ExamineCfg(cfg,"UNPACK \"");
	if (!s) return 0;
	cpytolaina(unpcmd,s);

	s=ExamineCfg(cfg,"BAD ");
	if (!s) return 0;
	getbasei(s,&bad);

	s=ExamineCfg(cfg,"NETMAIL ");
	if (!s) return 0;
	getbasei(s,&netm);

	s=ExamineCfg(cfg,"CONVERSION ");
	if (s) {
		sprintf(buf,"%s/data/conversiontable%2.2d.dat",getenv("DAYDREAM"),atoi(s));
		i=open(buf,O_RDONLY);
		if (i > -1) {
			read(i,&ctable,256);
			close(i);
			usetab=1;
		}
	}
	s=ExamineCfg(cfg,"OUT:\n");
	if (!s) return 0;
	outlist=NewList();
	while(*s!='~') {
		struct onodeinfo *on;
		struct aka ak;
		int akak;
		char buf[1024];
		char *t;
		char og[1024];
		
		on=malloc(sizeof(struct onodeinfo));
		s=strtoaka(s,&ak);
		if (!s) break;
		s++;
		on->NI_Zone=ak.zone;
		on->NI_Net=ak.net;
		on->NI_Node=ak.node;
		on->NI_Point=ak.point;
		on->NI_Type=*s;
		s+=2;
		on->NI_Packer=atoi(s);
		while(*s!=' ') s++;
		while(*s==' ') s++;

		akak=atoi(s);
		while(*s!=' ') s++;
		while(*s==' ') s++;
		sprintf(buf,"\nAKA%d ",akak);
		t=ExamineCfg(cfg,buf);
		if (!t) break;
		strtoaka(t,&ak);
		on->NI_OZone=ak.zone;
		on->NI_ONet=ak.net;
		on->NI_ONode=ak.node;
		on->NI_OPoint=ak.point;
		on->NI_Akak=akak;
		
		akak=atoi(s);
		while(*s!=10) s++;
		s++;
		on->NI_Msgs=on->NI_Sendflag=on->NI_Conv=0;
		
		if (akak) {
			sprintf(buf,"%s/data/conversiontable%2.2d.dat",getenv("DAYDREAM"),akak);
			i=open(buf,O_RDONLY);
			if (i > -1) {
				read(i,&on->NI_ConvTable,256);
				close(i);
				on->NI_Conv=1;
			} else {
				perror(buf);
			}
		}


		if (on->NI_Akak==1) {
			sprintf(og,"%s/",outgoing);
		} else {
			sprintf(og,"%s.%03x/",outgoing,on->NI_Zone);
		}
		mkdir(og,0775);
		
		if (on->NI_Point) {
			sprintf(buf,"%s%04x%04x.pnt/",og,on->NI_Net,on->NI_Node);
			mkdir(buf,0775);
		}		
		sprintf(on->NI_Pktname,"%08x.pkt",dd_getfidounique());

		on->NI_FHandle=open(on->NI_Pktname,O_WRONLY|O_CREAT|O_TRUNC,0664);
		if (on->NI_FHandle < 0) {
			perror(on->NI_Pktname);
			break;
		}
		
		makeftsh(on);
		close(on->NI_FHandle);
		AddTail(outlist,(struct Node *)on);
	}
	return 1;
}

static void makeftsh(struct onodeinfo *on)
{
	struct ftsheader mh;
	struct tm *tm;
	time_t tim=time(0);
	
	memset(&mh,0,sizeof(struct ftsheader));

	mh.origNode=on->NI_ONode;
	mh.destNode=on->NI_Node;
	mh.origNet=on->NI_ONet;
	mh.destNet=on->NI_Net;
	mh.baud=9600;
	mh.id=0x0002;
	tm=localtime(&tim);
	mh.year=tm->tm_year+1900;
	mh.month=tm->tm_mon;
	mh.day=tm->tm_mday;
	mh.hour=tm->tm_hour;
	mh.minute=tm->tm_min;
	mh.second=tm->tm_sec;
	mh.origZoneq=on->NI_OZone;
	mh.destZoneq=on->NI_Zone;
	mh.origZone=on->NI_OZone;
	mh.destZone=on->NI_Zone;
	mh.origPoint=on->NI_OPoint;
	mh.destPoint=on->NI_Point;
	write(on->NI_FHandle,&mh,sizeof(struct ftsheader));
}

static void cpytolaina(char *s, char *t)
{
	while(*t!='\"' && *t) *s++=*t++;
	*s=0;
}

static char *strtoaka(char *s, struct aka *ak)
{
	char *t;
	char cb[1024];
	ak->zone=ak->net=ak->node=ak->point=0;
	
	if (s && *s==' ') while(*s==' ') s++;
	
	t=cb;
	while(isdigit(*s)) *t++=*s++;
	*t=0;
	ak->zone=atoi(cb);
	if (*s!=':') return 0; else s++;
	
	t=cb;
	while(isdigit(*s)) *t++=*s++;
	*t=0;
	ak->net=atoi(cb);
	if (*s!='/') return 0; else s++;

	t=cb;
	while(isdigit(*s)) *t++=*s++;
	*t=0;
	ak->node=atoi(cb);
	if (!*s || *s==13 || *s==10 || *s==' ') return s; else s++;

	t=cb;
	while(isdigit(*s)) *t++=*s++;
	*t=0;
	ak->point=atoi(cb);

	return s;
}

static char *ExamineCfg(char *hay, const char *need)
{
	const char *s;
	for (;;) {
		s = need;
		if (*hay == 0) return 0;
		if (*hay == ';') { 
			while(*hay != 10) {
				if(*hay == 0) 
					return 0;
				hay++;
			}
			continue;
		}
		for (;;) {
			if (*s++ == *hay) {
				hay++;
				if (*s == 0) 
					return hay;
			} else {
				if ((s-1) == need) 
					hay++;
				break;
			}
		}
	}
}

static struct List * NewList(void)
{
	struct List *myl;
	
	myl=(struct List *)malloc(sizeof(struct List));
	myl->lh_Head=(struct Node *)&myl->lh_Tail;
	myl->lh_Tail=0;
	myl->lh_TailPred=(struct Node *)&myl->lh_Head;
	return myl;
}  

static void AddTail(struct List *myl, struct Node *myn)
{
	struct Node *tnod;
	
	tnod=myl->lh_TailPred;
	tnod->ln_Succ=myn;
	myl->lh_TailPred=myn;
	myn->ln_Succ=(struct Node *)&myl->lh_Tail;
	myn->ln_Pred=tnod;
}

static void ftndate(time_t t, char *buf)
{
        struct tm *ptm;
                
        ptm=localtime(&t);
        sprintf(buf,"%02d %s %02d  %02d:%02d:%02d",ptm->tm_mday,
                months[ptm->tm_mon],ptm->tm_year%100,
                ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
}    
