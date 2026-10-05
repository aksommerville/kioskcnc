#ifndef KIOSKCNC_H
#define KIOSKCNC_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "poller.h"
#include "savewatch.h"
#include "output.h"
#include "foutput.h"

extern struct g {

  const char *exename;
  const char *host;
  const char *remote;
  int zap_saves;
  int list_saves;
  
  volatile int sigc;
  struct poller *poller;
  struct savewatch *savewatch;
  struct output *output;
  struct foutput *foutput;
} g;

/* savefiles.c
 * Contains this one list of partial paths, terminated by a null.
 * Paths start after HOME, ie typically the first component is "proj".
 */
extern const char *savefilev[];

int zap_saves();
int list_saves();

double nowf();
int file_read(void *dstpp,const char *path);
int file_write(const char *path,const void *src,int srcc);

#endif
