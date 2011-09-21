#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ddlib.h>
#include <ddcommon.h>

int main(int argc, char *argv[])
{
	struct DayDream_Conference *confs;
	struct DayDream_Conference *conf;
	struct DayDream_MsgBase *base;
		
	confs=(struct DayDream_Conference *)dd_getconfdata();
	
	if (!confs) exit (0);
	conf=confs;

	while(1)
	{
		struct stat st;
		struct DayDream_Message msg;
		int bcnt;
		
		if (conf->CONF_NUMBER==255) break;
		
		/* FIXME: Very suspicious code */
		base = (struct DayDream_MsgBase *) conf + 1;
		bcnt=conf->CONF_MSGBASES;

		for (bcnt = conf->CONF_MSGBASES; bcnt ;bcnt--, base++) {
			int basefd;
			struct DayDream_MsgPointers mp;
			char wubuf[1024];
			
			sprintf(wubuf,"%s/messages/base%3.3d/msgbase.dat",
					conf->CONF_PATH,base->MSGBASE_NUMBER);			
			stat(wubuf,&st);
			if (st.st_size < sizeof(struct DayDream_Message))
			       	continue;

			basefd = open(wubuf, O_RDONLY);
			if (basefd < 0) 
				continue;
			
			read(basefd,&msg,sizeof(struct DayDream_Message));
			mp.msp_low=msg.MSG_NUMBER;
			dd_lseek(basefd, -sizeof(struct DayDream_Message), SEEK_END);
			read(basefd,&msg,sizeof(struct DayDream_Message));
			mp.msp_high=msg.MSG_NUMBER;

			close(basefd);

			sprintf(wubuf,"%s/messages/base%3.3d/msgbase.ptr",conf->CONF_PATH,base->MSGBASE_NUMBER);
			basefd=open(wubuf,O_WRONLY);
			if (basefd < 0) continue;
			write(basefd,&mp,sizeof(struct DayDream_MsgPointers));
			close(basefd);

		}
		/* FIXME: Very suspicious code */
		conf = (struct DayDream_Conference *) base;
	}
	return 0;
}
