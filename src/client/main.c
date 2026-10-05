#include "kioskcnc.h"
#include <signal.h>
#include <unistd.h>

struct g g={0};

/* Configure.
 * Always sets (g.exename), even on errors.
 */
 
static void print_help() {
  fprintf(stderr,"Usage: %s [OPTIONS]\n",g.exename);
  fprintf(stderr,
    "Endpoint daemon to monitor my saved games and send to a server when they change.\n"
    "OPTIONS:\n"
    "  --help                Print this message and exit.\n"
    "  --zap-saves           Instead of normal operation, prompt and then delete all save files.\n"
    "  --list-saves          Instead of normal operation, show the set of files I'm going to monitor.\n"
    "  --host=MY_HOST_NAME   Set host name. Normally `uname -n`. Can also use env HOST.\n"
    "  --remote=HOST:PORT    Server to send saved games to. If unset, save them locally under data/.\n"
  );
}
 
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
    if (!strcmp(arg,"--zap-saves")) {
      g.zap_saves=1;
      return 0;
    }
    if (!strcmp(arg,"--list-saves")) {
      g.list_saves=1;
      return 0;
    }
    if (!strcmp(arg,"--help")) {
      print_help();
      return -2;
    }
    
    fprintf(stderr,"%s: Unexpected argument '%s'\n",g.exename,arg);
    return -2;
  }
  if (!g.host||!g.host[0]) g.host="unknown";
  return 0;
}

/* Cleanup.
 */
 
static void cleanup() {
  savewatch_del(g.savewatch); g.savewatch=0;
  poller_del(g.poller); g.poller=0;
  output_del(g.output); g.output=0;
  foutput_del(g.foutput); g.foutput=0;
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
  
  if (g.remote&&g.remote[0]) {
    if (!(g.output=output_new(g.remote))) return -1;
  } else {
    if (!(g.foutput=foutput_new())) return -1;
  }
  
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
  
  /* Alternate modes.
   */
  if (g.zap_saves) {
    if ((err=zap_saves())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error zapping saves.\n",g.exename);
      return 1;
    }
    return 0;
  }
  if (g.list_saves) {
    if ((err=list_saves())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error listing saves.\n",g.exename);
      return 1;
    }
    return 0;
  }
  
  /* Normal daemon mode.
   */
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
