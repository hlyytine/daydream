#include <EZ.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#define INMAIN
#include "data.h"

void AddTail(struct List *, struct Node *);
struct List *NewList(void);

void changesize(void);
void listsel(void);
void openview(void);
void closeview(void);
void chat(void);
void kick(void);
void edit(void);
void quit(void);
void buildnodelist(void);
void radiocall(EZ_Widget *);
void mytimer(EZ_Timer *tim,void *, int);

int siz=1;
struct DayDream_MainConfig maincfg;
struct DayDream_Multinode *nodes;
char *nodestrs[256];
int vmode=0;

char *viewnames[] = { "Last uploads", "Last callers", "User info", "About" };

struct List *nodelist;


struct nle {
	struct Node fhead;
	char nle_whoinfo[140];
	int nle_num;
	struct DayDream_Multinode *nle_mn;
	pid_t snooper;
};

struct nle *activenle=0;

static int bn_add(struct DayDream_Multinode *, int);
static int initgui(void);
static int isnode(int, struct DayDream_NodeInfo *);
static int loaddata(void);

int readconfig(void);

int main(int argc, char *argv[])
{
	if (!loaddata()) exit(1);
	if (!readconfig()) exit(1);
	
	buildnodelist();
	EZ_Initialize(argc,argv,0);
	initgui();

	EZ_EventMainLoop();

	return 0;
}

static int loaddata(void)
{
	int fd;
	char foo[1024];
	struct stat fib;

	char *s;	
	sprintf(foo,"%s/data/daydream.dat",getenv("DAYDREAM"));
	fd=open(foo,O_RDONLY);
	if (fd < 0) return 0;
	read(fd,&maincfg,sizeof(struct DayDream_MainConfig));
	close(fd);

	sprintf(foo,"%s/data/multinode.dat",getenv("DAYDREAM"));
	fd=open(foo,O_RDONLY);
	if (fd==-1) return 0;

	fstat(fd,&fib);
	
	nodes=(struct DayDream_Multinode *)malloc(fib.st_size+2);
	read(fd,nodes,fib.st_size);
	close(fd);
	s=(char *)nodes;
	s[fib.st_size]=0;

	return 1;

}

void buildnodelist()
{
	struct DayDream_Multinode *cn;
	
	cn=nodes;
	
	nodelist=(struct List *)NewList();
	while(cn->MULTI_NODE)
	{
		if (cn->MULTI_NODE == 253) {
			int j;
			int i=maincfg.CFG_TELNET1ST;
			j=maincfg.CFG_TELNETMAX;
			
			while(j) {
				j--;
				bn_add(cn,i);
				i++;
			}
		} else if (cn->MULTI_NODE == 254) {
			int j;
			int i=maincfg.CFG_LOCAL1ST;
			j=maincfg.CFG_LOCALMAX;
			
			while(j) {
				j--;
				bn_add(cn,i);
				i++;
			}
		} else {
			bn_add(cn,cn->MULTI_NODE);
		}
		cn++;
	}
	return;
}

int buildnodew(int mode)
{
	struct nle *myn;
	char **ptr;
	int cnt=0;
	int nodefd;
	struct DayDream_NodeInfo myyn;
	struct userbase nuser;
	char *usern, *orgn;
	char *activity;
	char dabps[20];
	char keimobuf[1024];
	
	ptr=nodestrs;
	
	myn=(struct nle *)nodelist->lh_Head;
	
	while(myn->fhead.ln_Succ)
	{
		memset(&myyn,0,sizeof(struct DayDream_NodeInfo));
		*ptr++=myn->nle_whoinfo;
		if (isnode(myn->nle_num,&myyn)) {
			if (myyn.ddn_userslot > -1) {
				sprintf(keimobuf,"%s/data/userbase.dat",getenv("DAYDREAM"));
				nodefd=open(keimobuf,O_RDONLY);
				lseek(nodefd,myyn.ddn_userslot*sizeof(struct userbase),SEEK_SET);
				read(nodefd,&nuser,sizeof(struct userbase));
				close(nodefd);
				if (maincfg.CFG_FLAGS & (1L<<1)) usern=nuser.user_handle; else usern=nuser.user_realname;
				if (maincfg.CFG_FLAGS & (1L<<2)) orgn=nuser.user_organization; else orgn=nuser.user_zipcity;
				activity=myyn.ddn_activity;
				sprintf(dabps,"%d",myyn.ddn_bpsrate);
			} else {
				usern=" "; orgn=" ";
				activity=myyn.ddn_activity;
				sprintf(dabps,"%d",myyn.ddn_bpsrate);
			}
		} else {
			usern=" "; orgn=" "; activity="Waiting for a call...";
			sprintf(dabps,"%d",0);
		}

		switch (myn->nle_mn->MULTI_TTYTYPE)
		{
			case 1:
				strcpy(dabps,"LOCAL");
			break;
			case 2:
				if (myyn.ddn_flags & (1L<<1)) {
					sprintf(dabps,"%d",myyn.ddn_bpsrate);
				} else {
					strcpy(dabps,"TELNET");
				}
			break;
		}
		sprintf(keimobuf,"%-2.2d %-20.20s %-25.25s %-27.27s %s",myn->nle_num,usern,orgn,activity,dabps);
		if (strcmp(keimobuf,myn->nle_whoinfo)) {
			strcpy(myn->nle_whoinfo,keimobuf);
			if (mode==1) EZ_ModifyListBoxItem(nodelistw,myn->nle_whoinfo,cnt);
		}
		cnt++;
		myn=(struct nle *)myn->fhead.ln_Succ;
	}
	if (mode!=1) {
		EZ_SetListBoxItems(nodelistw,nodestrs,cnt);
	}
	return cnt;
}

static int bn_add(struct DayDream_Multinode *mn, int nu)
{
	struct nle *nl;
	
	nl=(struct nle *)malloc(sizeof(struct nle));
	sprintf(nl->nle_whoinfo,"%-2.2d %-20.20s %-25.25s %-20.20s %s",nu," ", " ", " ", " ");
	nl->nle_num=nu;
	nl->nle_mn=mn;
	nl->snooper=0;
	AddTail(nodelist,(struct node *)nl);
}

static int initgui(void)
{
	int i;
	EZ_Widget *tmp;
	
	topw=EZ_CreateFrame(0,0);
	EZ_ConfigureWidget(topw,EZ_PADY,0,EZ_ORIENTATION,EZ_VERTICAL_TOP,EZ_SIDE,EZ_LEFT,0);

	upframew=EZ_CreateFrame(topw,0);
	EZ_ConfigureWidget(topw,EZ_IPADX,6,0);

	bbsnamew=EZ_CreateLabel(upframew,maincfg.CFG_BOARDNAME);
	EZ_ConfigureWidget(bbsnamew,EZ_BORDER_TYPE,EZ_BORDER_RIDGE,EZ_BORDER_WIDTH,1,EZ_WIDTH,500,0);
	sizecw=EZ_CreateCheckButton(upframew,"Show buttons",0,1,0,1);

	nodelistw=EZ_CreateListBox(topw,0,1);
	EZ_ConfigureWidget(nodelistw,EZ_HEIGHT,5+16*nlines,EZ_WIDTH,640,EZ_FONT_NAME,"-misc-fixed-medium-r-normal--14-130-75-75-c-70-iso8859-1",0);
	EZ_SetWidgetCallBack(nodelistw,listsel,0);
	
	botw=EZ_CreateFrame(topw,0);
	EZ_ConfigureWidget(botw,EZ_SIDE,EZ_LEFT,0);
	cmd1w=EZ_CreateFrame(botw,0);
	EZ_ConfigureWidget(cmd1w,EZ_ORIENTATION,EZ_HORIZONTAL,EZ_SIDE,EZ_LEFT,EZ_PADY,1,0);

	cmd1lw=EZ_CreateFrame(cmd1w,0);
	openvieww=EZ_CreateButton(cmd1lw,"Open view",0);
	EZ_ConfigureWidget(openvieww,EZ_WIDTH,80,EZ_HEIGHT,25,EZ_X,0,EZ_Y,0,0);
	EZ_SetWidgetCallBack(openvieww,openview,0);
	
	closevieww=EZ_CreateButton(cmd1lw,"Close view",0);
	EZ_ConfigureWidget(closevieww,EZ_WIDTH,80,EZ_HEIGHT,25,EZ_X,81,EZ_Y,0,0);
	EZ_SetWidgetCallBack(closevieww,closeview,0);
	chatw=EZ_CreateButton(cmd1lw,"Chat",0);
	EZ_ConfigureWidget(chatw,EZ_WIDTH,80,EZ_HEIGHT,25,EZ_X,0,EZ_Y,26,0);
	EZ_SetWidgetCallBack(chatw,chat,0);
	kickw=EZ_CreateButton(cmd1lw,"Kick user",0);
	EZ_ConfigureWidget(kickw,EZ_WIDTH,80,EZ_HEIGHT,25,EZ_X,81,EZ_Y,26,0);
	EZ_SetWidgetCallBack(kickw,kick,0);
	editw=EZ_CreateButton(cmd1lw,"Edit user",0);
	EZ_ConfigureWidget(editw,EZ_WIDTH,80,EZ_HEIGHT,25,EZ_X,0,EZ_Y,52,0);
	EZ_SetWidgetCallBack(editw,edit,0);
	quitw=EZ_CreateButton(cmd1lw,"Quit",0);
	EZ_ConfigureWidget(quitw,EZ_WIDTH,80,EZ_HEIGHT,25,EZ_X,81,EZ_Y,52,0);
	EZ_SetWidgetCallBack(quitw,quit,0);
	
	for (i=0; i < 4 ; i++) {
		radiow[i]=EZ_CreateRadioButton(cmd1lw,viewnames[i],-1,0,i);
		EZ_ConfigureWidget(radiow[i],EZ_X,170+i*120,EZ_Y,1,0);
		EZ_SetWidgetCallBack(radiow[i],radiocall,0);
	}
	EZ_SetRadioButtonGroupVariableValue(tmp,0);

	infow=EZ_CreateListBox(cmd1lw,0,1);
	EZ_ConfigureWidget(infow,EZ_Y,30,EZ_X,170,EZ_WIDTH,440,EZ_HEIGHT,5+16*isize,EZ_FONT_NAME,"-misc-fixed-medium-r-normal--14-130-75-75-c-70-iso8859-1",0);

	buildnodew(0);
	EZ_SetListBoxItems(infow,lulines,ilines);

	EZ_DisplayWidget(topw);
	EZ_CreateTimer(1,0,-1,mytimer,0,0);
}

void mytimer(EZ_Timer *tim,void *v, int i)
{
	struct stat st;
	
	buildnodew(1);
	if (vmode==2) listsel();
	fstat(clogfd,&st);
	if (oldlcsize!=st.st_size) {
		oldlcsize=st.st_size;
		makelclist();
		if (vmode == 1) EZ_SetListBoxItems(infow,lclines,ilines);
	}
	fstat(ulogfd,&st);
	if (oldlusize!=st.st_size) {
		oldlusize=st.st_size;
		makelulist();
		if (vmode == 0) EZ_SetListBoxItems(infow,lulines,ilines);
	}
	while ((waitpid(-1, NULL, WNOHANG)) > 0);	
}

void radiocall(EZ_Widget *wid)
{
	int i;
	char *serverinfo[]={ "DayDream BBS Server", "Written by Antti Häyrynen", " ", "BBS: +358-8-5409139", "E-Mail: hydra@pato.vaala.fi" };
	
	for (i=0;i<4;i++) if (wid==radiow[i]) break;
	
	switch (i)
	{
		case 0:
			EZ_SetListBoxItems(infow,lulines,ilines);
			break;
		case 1:
			EZ_SetListBoxItems(infow,lclines,ilines);
			break;
		case 2:
			vmode=2;
			listsel();
			break;
		case 3:
			EZ_SetListBoxItems(infow,serverinfo,5);
			break;
	
	}
	vmode=i;
}


void changesize(void)
{
	if (siz) siz=0; else siz=1;

}

static int isnode(int nod, struct DayDream_NodeInfo *ndnfo)
{
	char infoname[1024];
	int nodefd;
		
	sprintf(infoname, "%s/nodeinfo%d.data", DDTMP, nod);
	nodefd=open(infoname,O_RDONLY);
	if (nodefd!=-1) {
		read(nodefd,ndnfo,sizeof(struct DayDream_NodeInfo));
		close(nodefd);
		if(ispid(ndnfo->ddn_pid)) return 1;
	}
	return 0;
}

int ispid(pid_t pid)
{
	return kill(pid, 0) != -1;
}

void listsel(void)
{
	char *s;
	struct nle *myn;
	s=EZ_GetListBoxSelectedItem(nodelistw);
	if (s==0) return ;
	
	myn=(struct nle *)nodelist->lh_Head;
	
	while(myn->fhead.ln_Succ)
	{
		if (!strncmp(myn->nle_whoinfo,s,3)) {
			activenle=myn;
			if (vmode==2) userinfo();
			break;
		}
		myn=(struct nle *)myn->fhead.ln_Succ;
	}	
}

int userinfo()
{
	char uibuf[90*6];
	char *uilines[6];
	int i;
	char *nouser = "No user online";
	struct DayDream_NodeInfo myyn;
	char keimobuf[1024];
	int nodefd;
	struct userbase nuser;
	
	for (i=0; i<7; i++) {
		uilines[i]=&uibuf[i*60];
	}
	
		
	if (isnode(activenle->nle_num,&myyn) && myyn.ddn_userslot > -1) {
		sprintf(keimobuf,"%s/data/userbase.dat",getenv("DAYDREAM"));
		nodefd=open(keimobuf,O_RDONLY);
		lseek(nodefd,myyn.ddn_userslot*sizeof(struct userbase),SEEK_SET);
		read(nodefd,&nuser,sizeof(struct userbase));
		close(nodefd);
		sprintf(uilines[0],"U/L: %Lu / %d",nuser.user_ulbytes,nuser.user_ulfiles);
		sprintf(uilines[1],"D/L: %Lu / %d",nuser.user_dlbytes,nuser.user_dlfiles);
		sprintf(uilines[2],"Slot/Sec: %3d / %3d      Calls: %d",nuser.user_account_id,nuser.user_securitylevel,nuser.user_connections);
		sprintf(uilines[3],"Time left/limit: %3d / %3d  BPS: %d",myyn.ddn_timeleft/60,nuser.user_dailytimelimit,myyn.ddn_bpsrate);
		sprintf(uilines[4],"Pub/Prv msgs: %5d / %5d  %-21.21s",nuser.user_pubmessages,nuser.user_pvtmessages,nuser.user_computermodel);
		if (*myyn.ddn_pagereason) {
			sprintf(uilines[5],"Paged! Reason: %-30.30s",myyn.ddn_pagereason);
		} else {
			strcpy(uilines[5]," ");
		}
		EZ_SetListBoxItems(infow,uilines,6);
	} else {
		EZ_SetListBoxItems(infow,&nouser,1);
	}
}

int makelulist(void)
{
	int i;
	for (i=0;i<ilines;i++) {
		struct DD_UploadLog loge;
		struct userbase nuser;
		char keimobuf[1024];
		int nodefd;
		char *usern;
		
		lseek(ulogfd,-( (i+1) * sizeof(struct DD_UploadLog)),SEEK_END);
		read(ulogfd,&loge,sizeof(struct DD_UploadLog));

		sprintf(keimobuf,"%s/data/userbase.dat",getenv("DAYDREAM"));
		nodefd=open(keimobuf,O_RDONLY);
		lseek(nodefd,loge.UL_SLOT*sizeof(struct userbase),SEEK_SET);
		read(nodefd,&nuser,sizeof(struct userbase));
		close(nodefd);

		if (maincfg.CFG_FLAGS & (1L<<1)) 
			usern=nuser.user_handle;
		else
			usern=nuser.user_realname;
			
		sprintf(lulines[i],"%-20.20s %11d %2d %s",loge.UL_FILENAME,loge.UL_FILESIZE,loge.UL_CONF,usern);
	}
	lseek(ulogfd,0,SEEK_END);
}


int makelclist(void)
{
	int i;
	for (i=0;i<ilines;i++) {
		struct gcallerslog loge;
		struct userbase nuser;
		char keimobuf[1024];
		int nodefd;
		char *usern, *orgn;
		
		lseek(clogfd,-( (i+1) * sizeof(struct gcallerslog)),SEEK_END);
		read(clogfd,&loge,sizeof(struct gcallerslog));

		sprintf(keimobuf,"%s/data/userbase.dat",getenv("DAYDREAM"));
		nodefd=open(keimobuf,O_RDONLY);
		lseek(nodefd,loge.cl.cl_userid*sizeof(struct userbase),SEEK_SET);
		read(nodefd,&nuser,sizeof(struct userbase));
		close(nodefd);

		if (maincfg.CFG_FLAGS & (1L<<1)) 
			usern=nuser.user_handle;
		else
			usern=nuser.user_realname;

		if (maincfg.CFG_FLAGS & (1L<<2)) orgn=nuser.user_organization; else orgn=nuser.user_zipcity;
			
		sprintf(lclines[i],"#%03d %-20.20s %-20.20s %d mins",loge.cl_node,usern,orgn,(loge.cl.cl_logoff-loge.cl.cl_logon)/60);
	}
	lseek(clogfd,0,SEEK_END);
}

void openview(void)
{
	struct DayDream_NodeInfo ninfo;

	listsel();

	if (activenle && isnode(activenle->nle_num,&ninfo) && (!activenle->snooper || !ispid(activenle->snooper))) {
		activenle->snooper=execute(viewcmd,activenle->nle_num);
	}
}

void closeview(void)
{
	listsel();
	if (activenle && activenle->snooper &&ispid(activenle->snooper)) {
		kill(activenle->snooper,SIGHUP);
		activenle->snooper=0;
	}
}

void chat(void)
{
	struct DayDream_NodeInfo ninfo;
	struct dd_nodemessage dn;
	
	listsel();
	if (activenle && isnode(activenle->nle_num,&ninfo)) {
		openview();
		dn.dn_command=3;
		sendtosock(activenle->nle_num,&dn);
	}

}

void kick(void)
{
	struct DayDream_NodeInfo ninfo;
	struct dd_nodemessage dn;
	
	listsel();
	if (activenle && isnode(activenle->nle_num,&ninfo)) {
		dn.dn_command=4;
		sendtosock(activenle->nle_num,&dn);
	}

}
int sendtosock(int dnode, struct dd_nodemessage *dn)
{	
	int sock;
        struct sockaddr_un name;
     	
     	sprintf(name.sun_path, "%s/dd_sock%d", DDTMP, dnode);
	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) 
        name.sun_family = AF_UNIX;

        sendto(sock, dn, sizeof(struct dd_nodemessage), 0, (struct sockaddr *)&name, sizeof(struct sockaddr_un));
        close(sock);
	return 1;
}

void quit(void)
{
	exit(0);
}

void edit(void)
{
	struct DayDream_NodeInfo ninfo;
	struct dd_nodemessage dn;
	
	listsel();
	if (activenle && isnode(activenle->nle_num,&ninfo)) {
		openview();
		dn.dn_command=5;
		sendtosock(activenle->nle_num,&dn);
	}


}
