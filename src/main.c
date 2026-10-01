#include "kioskcnc.h"

struct g g={0};

/* Configure.
 * Always sets (g.exename), even on errors.
 */
 
static int configure(int argc,char **argv) {
  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) g.exename=argv[0];
  else g.exename="kioskcnc";
  int argi=1;
  while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    
    fprintf(stderr,"%s: Unexpected argument '%s'\n",g.exename,arg);
    return -2;
  }
  return 0;
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err=configure(argc,argv);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error starting up.\n",g.exename);
    return 1;
  }
  //TODO
  return 0;
}
