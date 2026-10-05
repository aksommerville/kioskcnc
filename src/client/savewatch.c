#include "kioskcnc.h"
#include <sys/inotify.h>
#include <unistd.h>

struct savewatch {
  int infd;
  int dirty;
  double waittime; // If (dirty), wait until this absolute time and then report all changes.
  struct watch {
    int wd;
    int dirty;
    char *path;
    char *base; // WEAK, points into (path).
    int basec;
  } *watchv;
  int watchc,watcha;
  int err; // Sticky error used during construction as a convenience.
  const char *home; // from env
};

/* Delete.
 */
 
static void watch_cleanup(struct watch *watch) {
  if (watch->path) free(watch->path);
}
 
void savewatch_del(struct savewatch *savewatch) {
  if (!savewatch) return;
  if (savewatch->infd>=0) close(savewatch->infd);
  if (savewatch->watchv) {
    while (savewatch->watchc-->0) watch_cleanup(savewatch->watchv+savewatch->watchc);
    free(savewatch->watchv);
  }
  free(savewatch);
}

/* Add one file.
 * (path) gets an implicit "~/" prefix.
 * Errors are recorded in (savewatch->err) so we can grab them all at the end of the batch.
 * We assume that all interesting files are in different directories, so they never share a (wd).
 * We'll be a bit redundant if we're ever asked to watch siblings.
 */
 
static void savewatch_add(struct savewatch *savewatch,const char *rpath) {
  if (savewatch->err<0) return;
  
  /* Compose the full path.
   */
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%s/%s",savewatch->home,rpath);
  if ((pathc<1)||(pathc>=sizeof(path))) { savewatch->err=-1; return; }
  
  /* Separate dirname and basename.
   */
  int sepp=pathc;
  while ((sepp>=0)&&(path[--sepp]!='/')) ;
  if (sepp<2) { savewatch->err=-1; return; }
  const char *base=path+sepp+1;
  int basec=pathc-sepp-1;
  if (basec<1) { savewatch->err=-1; return; }
  
  /* Allocate a new watch record.
   */
  if (savewatch->watchc>=savewatch->watcha) {
    int na=savewatch->watcha+16;
    if (na>INT_MAX/sizeof(struct watch)) { savewatch->err=-1; return; }
    void *nv=realloc(savewatch->watchv,sizeof(struct watch)*na);
    if (!nv) { savewatch->err=-1; return; }
    savewatch->watchv=nv;
    savewatch->watcha=na;
  }
  struct watch *watch=savewatch->watchv+savewatch->watchc++;
  memset(watch,0,sizeof(struct watch));
  
  /* Copy path.
   */
  if (!(watch->path=malloc(pathc+1))) { savewatch->err=-1; return; }
  memcpy(watch->path,path,pathc+1);
  watch->base=watch->path+sepp+1;
  watch->basec=basec;
  
  /* Register with inotify.
   */
  path[sepp]=0;
  if ((watch->wd=inotify_add_watch(savewatch->infd,path,IN_MODIFY|IN_ATTRIB|IN_MOVED_TO|IN_CREATE))<0) {
    fprintf(stderr,"%s: inotify_add_watch failed. Does the directory exist?\n",path);
    savewatch->err=-1;
    return;
  }
}

/* New.
 */

struct savewatch *savewatch_new() {
  struct savewatch *savewatch=calloc(1,sizeof(struct savewatch));
  if (!savewatch) return 0;
  
  if (!(savewatch->home=getenv("HOME"))) savewatch->home="/home/andy";
  
  if ((savewatch->infd=inotify_init())<0) {
    savewatch_del(savewatch);
    return 0;
  }
  
  const char **subpathp=savefilev;
  for (;*subpathp;subpathp++) savewatch_add(savewatch,*subpathp);
  
  if (savewatch->err<0) {
    savewatch_del(savewatch);
    return 0;
  }
  return savewatch;
}

/* Set our dirty flags for later review.
 */
 
static void savewatch_set_dirty(struct savewatch *savewatch,struct watch *watch) {
  watch->dirty=1;
  if (!savewatch->dirty) {
    savewatch->dirty=1;
    savewatch->waittime=nowf()+0.500;
  }
}

/* Read events from inotify response.
 */
 
static int savewatch_cb_read(int fd,void *userdata,const void *src,int srcc) {
  struct savewatch *savewatch=userdata;
  int srcp=0;
  while (srcp<srcc) {
    const struct inotify_event *event=(struct inotify_event*)((char*)src+srcp);
    int evlen=sizeof(struct inotify_event)+event->len;
    srcp+=evlen;
    if (srcp>srcc) return -1; // This could happen if we have more events pending than fit in one buffer. I say it's ok to fail in that case.
    const char *base=event->name;
    int basec=0;
    while ((basec<event->len)&&base[basec]) basec++;
    
    /* We do no processing at this time.
     * Just search for this file, mark it, and mark us at the context level as having something dirty.
     * inotify events travel in herds sometimes so it's good to give them some breathing room.
     */
    struct watch *watch=savewatch->watchv;
    int i=savewatch->watchc;
    for (;i-->0;watch++) {
      if (watch->wd!=event->wd) continue;
      if (watch->basec!=basec) continue;
      if (memcmp(watch->base,base,basec)) continue;
      savewatch_set_dirty(savewatch,watch);
    }
  }
  return 0;
}

/* Register files with poller.
 */
 
int savewatch_register_files(struct savewatch *savewatch,struct poller *poller) {
  if (!savewatch||!poller) return -1;
  if (savewatch->infd<0) return -1;
  return poller_register_file_buffered(poller,savewatch->infd,savewatch,savewatch_cb_read,0);
}

/* Sync one dirty file.
 */
 
static int savewatch_sync(struct savewatch *savewatch,struct watch *watch) {
  void *serial=0;
  int serialc=file_read(&serial,watch->path);
  if (serialc<0) {
    fprintf(stderr,"%s: Read failed, can't sync via savewatch.\n",watch->path);
    return 0;
  }
  int err;
  if (g.output) {
    err=output_queue_event_loose(g.output,watch->path,serial,serialc);
  } else if (g.foutput) {
    err=foutput_queue_event_loose(g.foutput,watch->path,serial,serialc);
  } else {
    fprintf(stderr,"%s: No output mode was configured.\n",__func__);
    err=-2;
  }
  free(serial);
  return err;
}

/* Routine update.
 */
 
int savewatch_update(struct savewatch *savewatch) {
  if (!savewatch||!savewatch->dirty) return 0;
  double pending_time=savewatch->waittime-nowf();
  if (pending_time>10.0) { // System clock changed or something?
    fprintf(stderr,"%s:%d:WARNING: Improbable deferral time %.03f s for savewatch. Syncing now.\n",__FILE__,__LINE__,pending_time);
    pending_time=-1.0;
  }
  if (pending_time>0.0) return 0;
  savewatch->dirty=0;
  struct watch *watch=savewatch->watchv;
  int i=savewatch->watchc;
  for (;i-->0;watch++) {
    if (!watch->dirty) continue;
    watch->dirty=0;
    int err=savewatch_sync(savewatch,watch);
    if (err<0) return err;
  }
  return 0;
}
