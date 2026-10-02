#include "kioskcnc.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* savefiles.c
 * Registry of all the saved games we monitor.
 * It's OK for a file not to exist, but its directory must.
 * Notes regarding format per file are in server/scoreboard.js.
 */

const char *savefilev[]={
  "proj/all52/out/all52-linux.save",
  "proj/apothecary/out/apothecary-linux.save",
  "proj/bellacopia/out/bellacopia-linux.save",
  "proj/cherteau/out/cherteau-linux.save",
  "proj/hummfu/out/hummfu-linux.save",
  "proj/iggle/out/iggle-linux.save",
  "proj/inversion/out/inversion-linux.save",
  "proj/kabobblin/out/kabobblin-linux.save",
  "proj/kleptomania/out/kleptomania-linux.save",
  "proj/licensetoilluse/out/licensetoilluse-linux.save",
  "proj/penance/out/penance-linux.save",
  "proj/presto/out/presto-linux.save",
  "proj/queenofclocks/out/queenofclocks-linux.save",
  "proj/samsam/out/samsam-linux.save",
  "proj/upsy-downsy/out/upsy-downsy-linux.save", // NB name collapses to "upsy".
  "proj/vexularg/out/vexularg-linux.save",
  "proj/zerosigma/out/wicked_garden-linux.save",
  "proj/wishbone/out/wishbone-linux.save",
  "proj/xrm/out/xrm-linux.save",
  "proj/younap/out/younap-linux.save",
  "proj/zennoniwa/out/zennoniwa-linux.save",
0};

/* Assess one subpath.
 * 'f' if it exists, 'd' if its directory exists, otherwise NUL.
 */
 
static char judge_save_path(const char *subpath) {
  const char *home=getenv("HOME");
  if (!home||!home[0]) home="/home/andy";
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%s/%s",home,subpath);
  if ((pathc<1)||(pathc>=sizeof(path))) return 0;
  struct stat st={0};
  if (stat(path,&st)>=0) {
    if (S_ISREG(st.st_mode)) return 'f';
    return 0; // Save paths must name a regular file. If it's a directory or something, that's a serious error.
  }
  int p=pathc;
  while (--p>=0) {
    if (path[p]=='/') {
      path[p]=0;
      break;
    }
  }
  if (stat(path,&st)>=0) {
    if (S_ISDIR(st.st_mode)) return 'd';
  }
  return 0;
}

/* Alternate main mode: --zap-saves
 */
 
int zap_saves() {
  
  #define LIMIT 128
  char *realv[LIMIT];
  int realc=0;
  int ckc=0;
  
  const char *home=getenv("HOME");
  if (!home||!home[0]) home="/home/andy";
  const char **p=savefilev;
  for (;*p;p++) {
    ckc++;
    const char *subpath=*p;
    char path[1024];
    int pathc=snprintf(path,sizeof(path),"%s/%s",home,subpath);
    if ((pathc<1)||(pathc>=sizeof(path))) continue;
    struct stat st={0};
    if (stat(path,&st)<0) continue;
    if (!S_ISREG(st.st_mode)) continue;
    if (realc>=LIMIT) break;
    if (!(realv[realc++]=strdup(path))) return -1; // let memory leak
  }
  #undef LIMIT
  
  if (!realc) {
    fprintf(stderr,"%s: No save files. Checked %d.\n",g.exename,ckc);
    return 0;
  }
  
  fprintf(stderr,"%s: Ready to delete %d files, from a possible %d.\n",g.exename,realc,ckc);
  int i=0; for (;i<realc;i++) {
    fprintf(stderr,"  %s\n",realv[i]);
  }
  fprintf(stderr,"Proceed? [y/N] ");
  char rsp[16];
  int rspc=read(STDIN_FILENO,rsp,sizeof(rsp));
  if ((rspc>=1)&&((rsp[0]=='y')||(rsp[0]=='Y'))) {
  
    for (i=0;i<realc;i++) {
      unlink(realv[i]);
    }
  
  } else {
    fprintf(stderr,"%s: Aborted\n",g.exename);
  }
  return 0;
}

/* Alternate main mode: --list-saves
 */
 
int list_saves() {
  fprintf(stdout,"%s: All saved games we will monitor. 'ok' if file exists. '--' if directory exists. '!!!' if no directory.\n",g.exename);
  fprintf(stdout,"If there's any '!!!', you must fix them before running the daemon.\n");
  const char **p=savefilev;
  for (;*p;p++) {
    const char *path=*p;
    const char *judgment="";
    switch (judge_save_path(path)) {
      case 0: judgment="!!!"; break;
      case 'd': judgment="--"; break;
      case 'f': judgment="ok"; break;
      default: judgment="???"; break;
    }
    fprintf(stdout,"%3s %s\n",judgment,path);
  }
  return 0;
}
