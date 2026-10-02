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

extern struct g {

  const char *exename;
  const char *host;
  const char *remote;
  
  volatile int sigc;
  struct poller *poller;
  struct savewatch *savewatch;
  struct output *output;
} g;

double nowf();
int file_read(void *dstpp,const char *path);

#endif
