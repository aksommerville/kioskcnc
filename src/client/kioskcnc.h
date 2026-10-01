#ifndef KIOSKCNC_H
#define KIOSKCNC_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "poller.h"
#include "savewatch.h"

extern struct g {

  const char *exename;
  
  volatile int sigc;
  struct poller *poller;
  struct savewatch *savewatch;
} g;

double nowf();
int file_read(void *dstpp,const char *path);

#endif
