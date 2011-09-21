#include <EZ.h>

#ifdef INMAIN
#define glob
#else
#define glob extern
#endif

glob EZ_Widget *topw;
glob EZ_Widget *upframew;
glob EZ_Widget *bbsnamew;
glob EZ_Widget *sizecw;
glob EZ_Widget *nodelistw;
glob EZ_Widget *botw;
glob EZ_Widget *cmd1w;
glob EZ_Widget *cmd2w;
glob EZ_Widget *cmd1lw;
glob EZ_Widget *openvieww;
glob EZ_Widget *closevieww;
glob EZ_Widget *cmd2lw;
glob EZ_Widget *chatw;
glob EZ_Widget *kickw;
glob EZ_Widget *editw;
glob EZ_Widget *quitw;
glob EZ_Widget *radiow[4];
glob EZ_Widget *infow;

glob int nlines;
glob int isize;
glob int ilines;

glob char *cfg;

glob char *lcmem;
glob char *lumem;

glob char **lclines;
glob char **lulines;

glob int clogfd;
glob int ulogfd;

glob int oldlcsize;
glob int oldlusize;

glob char viewcmd[512];

char *ecfg(char *, char *);
void CpyToLAINA(char *, char *);

