#include "kioskcnc.h"
#include "serial/serial.h"
#include <time.h>

struct foutput {
  int dummy; // I guess we ought to accept the destination path at the command line? Meh.
};

/* Cleanup.
 */
 
void foutput_del(struct foutput *foutput) {
  if (!foutput) return;
  free(foutput);
}

/* Init.
 */
 
struct foutput *foutput_new() {
  struct foutput *foutput=calloc(1,sizeof(struct foutput));
  if (!foutput) return 0;
  return foutput;
}

/* Path for save file.
 */
 
static int foutput_compose_save_path(char *dst,int dsta,const struct foutput *foutput,const char *gamepath,const void *src,int srcc) {
  
  // Take the leading C identifier of (gamepath)'s basename.
  // Same as gets written as "file" in the JSON.
  const char *base=gamepath;
  int basec=0,pathp=0,baseok=1;
  for (;gamepath[pathp];pathp++) {
    if (gamepath[pathp]=='/') {
      base=gamepath+pathp+1;
      basec=0;
      baseok=1;
    } else if (baseok&&(
      ((gamepath[pathp]>='a')&&(gamepath[pathp]<='z'))||
      ((gamepath[pathp]>='A')&&(gamepath[pathp]<='Z'))||
      ((gamepath[pathp]>='0')&&(gamepath[pathp]<='9'))||
      (gamepath[pathp]=='_')
    )) {
      basec++;
    } else {
      baseok=0;
    }
  }
  if (basec<1) return -1;
  
  // And the current local time, always helpful.
  // This is not written in the JSON, so the path is the only place it gets recorded.
  // (and the file's ctime, I guess, if we want to depend on that. but we don't)
  time_t now=time(0);
  struct tm tm={0};
  localtime_r(&now,&tm);
  int year=1900+tm.tm_year;
  int month=1+tm.tm_mon;
  int day=tm.tm_mday;
  int hour=tm.tm_hour;
  int minute=tm.tm_min;
  int second=tm.tm_sec;
  
  // We don't use (src), (foutput), or any globals. But those are available if we ever want.
  
  int dstc=snprintf(dst,dsta,"data/%04d%02d%02d%02d%02d%02d-%.*s.json",year,month,day,hour,minute,second,basec,base);
  if (dstc==dsta) return -1;
  return dstc;
}

/* Digest one save file and write it out.
 */

int foutput_queue_event_loose(struct foutput *foutput,const char *path,const void *src,int srcc) {
  if (!foutput) return -1;
  struct sr_encoder encoder={0};
  if (output_encode_event(&encoder,path,src,srcc)<0) {
    sr_encoder_cleanup(&encoder);
    return -1;
  }
  char savepath[1024];
  int savepathc=foutput_compose_save_path(savepath,sizeof(savepath),foutput,path,src,srcc);
  if ((savepathc<1)||(savepathc>=sizeof(savepath))) {
    sr_encoder_cleanup(&encoder);
    return -1;
  }
  int err=file_write(savepath,encoder.v,encoder.c);
  sr_encoder_cleanup(&encoder);
  return err;
}
