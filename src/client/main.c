#include "kioskcnc.h"
#include <signal.h>
#include <unistd.h>

struct g g={0};

/* Configure.
 * Always sets (g.exename), even on errors.
 */
 
static int configure(int argc,char **argv) {
  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) g.exename=argv[0];
  else g.exename="kioskcnc";
  g.host=getenv("HOST"); // Not a standard thing.
  int argi=1;
  while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    
    if (!memcmp(arg,"--host=",7)) {
      g.host=arg+7;
      continue;
    }
    if (!memcmp(arg,"--remote=",9)) {
      g.remote=arg+9;
      continue;
    }
    
    fprintf(stderr,"%s: Unexpected argument '%s'\n",g.exename,arg);
    return -2;
  }
  if (!g.host||!g.host[0]) g.host="unknown";
  if (!g.remote||!g.remote[0]) {
    fprintf(stderr,"%s:ERROR: Please specify remote host as '--remote=HOST:PORT'\n",g.exename);
    return -2;
  }
  return 0;
}

/* Cleanup.
 */
 
static void cleanup() {
  savewatch_del(g.savewatch); g.savewatch=0;
  poller_del(g.poller); g.poller=0;
  output_del(g.output); g.output=0;
}

/* Signals.
 */
 
static void rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++(g.sigc)>=3) {
        fprintf(stderr,"%s: Too many unprocessed signals.\n",g.exename);
        exit(1);
      } break;
  }
}

/* Init.
 */
 
static int init() {
  int err;
  
  signal(SIGINT,rcvsig);
  
  if (!(g.poller=poller_new())) return -1;
  
  if (!(g.savewatch=savewatch_new())) return -1;
  
  if (!(g.output=output_new(g.remote))) return -1;
  
  return 0;
}

/* Update.
 * Blocks.
 * Returns 0 to terminate normally, >0 to proceed, or <0 for fatal errors.
 */
 
static int update() {
  int err;
  if (g.sigc) return 0;
  poller_flush(g.poller);
  
  if (g.savewatch) {
    if ((err=savewatch_register_files(g.savewatch,g.poller))<0) return err;
  }
  if (g.output) {
    if ((err=output_register_files(g.output,g.poller))<0) return err;
  }
  
  if ((err=poller_update(g.poller,500))<0) return err;
  
  if (g.savewatch) {
    if ((err=savewatch_update(g.savewatch))<0) return err;
  }
  if (g.output) {
    if ((err=output_update(g.output))<0) return err;
  }
  
  if (g.sigc) return 0;
  return 1;
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err=configure(argc,argv);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error reading configuration.\n",g.exename);
    return 1;
  }
  if ((err=init())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error starting services.\n",g.exename);
    cleanup();
    return 1;
  }
  fprintf(stderr,"%s: Running. SIGINT to quit.\n",g.exename);
  for (;;) {
    if ((err=update())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error updating.\n",g.exename);
      cleanup();
      return 1;
    }
    if (!err) break;
  }
  cleanup();
  fprintf(stderr,"%s: Normal exit.\n",g.exename);
  return 0;
}
