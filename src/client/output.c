#include "kioskcnc.h"
#include "serial/serial.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

struct output {
  struct sockaddr_in saddr;
  int fd; // Can be <0; we'll retry connection forever.
  char *wbuf;
  int wbufa;
  int wbufp,wbufc;
  double next_connect_time; // Space connection attempts in time, don't retry every cycle.
};

/* Cleanup.
 */
 
void output_del(struct output *output) {
  if (!output) return;
  if (output->fd>=0) close(output->fd);
  if (output->wbuf) free(output->wbuf);
  free(output);
}

/* Evaluate hostname and port.
 * Populates (output->saddr).
 */
 
static int output_eval_host(struct output *output,const char *src) {
  if (!src) return -1;
  const char *port=0;
  int hostlen=0;
  while (src[hostlen]&&(src[hostlen]!=':')) hostlen++;
  if (!src[hostlen]) {
    fprintf(stderr,"%s: Must give both host and port for --remote\n",g.exename);
    return -2;
  }
  char zhost[256];
  if (hostlen>=sizeof(zhost)) return -1;
  memcpy(zhost,src,hostlen);
  zhost[hostlen]=0;
  port=src+hostlen+1;
  
  struct addrinfo hint={
    .ai_family=AF_INET,
    .ai_socktype=SOCK_STREAM,
    .ai_protocol=IPPROTO_TCP,
    .ai_flags=AI_ADDRCONFIG,
  };
  struct addrinfo *aistore=0;
  int err=getaddrinfo(zhost,port,&hint,&aistore);
  if (err<0) {
    return -1;
  }
  
  int ok=0;
  struct addrinfo *ai=aistore;
  for (;ai;ai=ai->ai_next) {
    if (ai->ai_addrlen!=sizeof(struct sockaddr_in)) continue;
    if (ai->ai_addr->sa_family!=AF_INET) continue;
    memcpy(&output->saddr,ai->ai_addr,sizeof(struct sockaddr_in));
    ok=1;
    break;
  }
  
  freeaddrinfo(aistore);
  return ok?0:-1;
}

/* New.
 */
 
struct output *output_new(const char *host_and_port) {
  struct output *output=calloc(1,sizeof(struct output));
  if (!output) return 0;
  output->fd=-1;
  if (output_eval_host(output,host_and_port)<0) {
    fprintf(stderr,"%s: Failed to resolve remote host '%s'\n",g.exename,host_and_port);
    output_del(output);
    return 0;
  }
  return output;
}

/* Content from server.
 * We don't care about the HTTP responses (maybe we should?).
 * But we do care about errors and closure, which report here as empty content.
 */
 
static int output_cb_read(int fd,void *userdata,const void *src,int srcc) {
  struct output *output=userdata;
  if (0&&srcc) { // XXX show me
    fprintf(stderr,"RECEIVED FROM REMOTE:\n%.*s\n-----\n",srcc,(char*)src);
  }
  if (!srcc) {
    if (output->fd>=0) {
      fprintf(stderr,"%s: Lost remote connection to %s.\n",g.exename,g.remote);
      close(output->fd);
      output->fd=-1;
      return 0;
    }
  }
  return 0;
}

/* Socket is writeable and we have something to write.
 */
 
static int output_cb_writeable(int fd,void *userdata) {
  struct output *output=userdata;
  if (output->wbufc<1) return 0; // Shouldn't happen.
  int err=write(output->fd,output->wbuf+output->wbufp,output->wbufc);
  if (err<=0) {
    fprintf(stderr,"%s: Write failed to %s, dropping connection.\n",g.exename,g.remote);
    close(output->fd);
    output->fd=-1;
    return 0;
  }
  fprintf(stderr,"OUT TO THE REMOTE:\n%.*s\n-----\n",err,output->wbuf+output->wbufp);
  if ((output->wbufc-=err)<=0) {
    output->wbufp=output->wbufc=0;
  } else {
    output->wbufp+=err;
  }
  return 0;
}

/* Attempt connection.
 */
 
static int output_connect(struct output *output) {
  if (output->fd>=0) return 0; // why did you call us...
  double now=nowf();
  if (now<output->next_connect_time) return -1; // Wait a little, don't spam it.
  output->next_connect_time=now+5.000; // If we fail, delay so long before retry.
  int fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
  if (fd<0) return -1;
  if (connect(fd,(struct sockaddr*)&output->saddr,sizeof(output->saddr))<0) {
    close(fd);
    return -1;
  }
  output->fd=fd;
  fprintf(stderr,"%s: Connected to %s\n",g.exename,g.remote);
  return 0;
}

/* Register files with poller.
 */

int output_register_files(struct output *output,struct poller *poller) {
  if (output->fd<0) {
    if (!output->wbufc) return 0;
    if (output_connect(output)<0) return 0;
  }
  if (poller_register_file_buffered(poller,output->fd,output,output_cb_read,output->wbufc?output_cb_writeable:0)<0) return -1;
  return 0;
}

/* Routine updates.
 */

int output_update(struct output *output) {
  return 0;
}

/* Queue raw text.
 */
 
static int output_queue_raw(struct output *output,const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (output->wbufp+output->wbufc>output->wbufa-srcc) {
    if (output->wbufp) {
      memmove(output->wbuf,output->wbuf+output->wbufp,output->wbufc);
      output->wbufp=0;
    }
    if (output->wbufc>output->wbufa-srcc) {
      if (output->wbufc>INT_MAX-srcc) return -1;
      int na=output->wbufc+srcc;
      if (na<INT_MAX-1024) na=(na+1024)&~1023;
      void *nv=realloc(output->wbuf,na);
      if (!nv) return -1;
      output->wbuf=nv;
      output->wbufa=na;
    }
  }
  memcpy(output->wbuf+output->wbufp+output->wbufc,src,srcc);
  output->wbufc+=srcc;
  return 0;
}

/* Queue formatted text.
 */

static int output_queue_decsint(struct output *output,int v) {
  char tmp[16];
  int tmpc=0;
  if (v==INT_MIN) v++;
  if (v<0) {
    tmp[tmpc++]='-';
    v=-v;
  }
  int digitc=1,limit=10;
  while (v>=limit) { digitc++; if (limit>INT_MAX/10) break; limit*=10; }
  int i=digitc;
  for (;i-->0;v/=10) tmp[tmpc+i]='0'+v%10;
  tmpc+=digitc;
  return output_queue_raw(output,tmp,tmpc);
}

/* Queue event.
 */
 
int output_queue_event(struct output *output,const char *body,int bodyc) {
  if (!output) return -1;
  if (!body) return -1;
  if (bodyc<0) { bodyc=0; while (body[bodyc]) bodyc++; }
  int bufc0=output->wbufc;
  if (output_queue_raw(output,"POST /event HTTP/1.1\r\nHost: ",-1)<0) goto _error_;
  if (output_queue_raw(output,g.host,-1)<0) goto _error_;
  if (output_queue_raw(output,"\r\nContent-Length: ",-1)<0) goto _error_;
  if (output_queue_decsint(output,bodyc)<0) goto _error_;
  if (output_queue_raw(output,"\r\n\r\n",4)<0) goto _error_;
  if (output_queue_raw(output,body,bodyc)<0) goto _error_;
  return 0;
 _error_:;
  output->wbufc=bufc0;
  return -1;
}

/* Decode Egg save file and reencode as JSON ready to send to POST /event.
 */
 
static int output_encode_event(struct sr_encoder *dst,const char *path,const uint8_t *src,int srcc) {
  int outerctx=sr_encode_json_object_start(dst,0,0);
  sr_encode_json_string(dst,"host",4,g.host,-1);
  
  // name is the leading C identifier of (path)'s basename.
  const char *base=path;
  int basec=0,pathp=0,baseok=1;
  for (;path[pathp];pathp++) {
    if (path[pathp]=='/') {
      base=path+pathp+1;
      basec=0;
      baseok=1;
    } else if (baseok&&(
      ((path[pathp]>='a')&&(path[pathp]<='z'))||
      ((path[pathp]>='A')&&(path[pathp]<='Z'))||
      ((path[pathp]>='0')&&(path[pathp]<='9'))||
      (path[pathp]=='_')
    )) {
      basec++;
    } else {
      baseok=0;
    }
  }
  if (basec<1) return -1;
  sr_encode_json_string(dst,"file",4,base,basec);
  
  /* (src) is an Egg save file, which we'll rewrite as a JSON object.
   */
  int bodyctx=sr_encode_json_object_start(dst,"body",4);
  int srcp=0;
  for (;;) {
    if (srcp>srcc-3) break;
    int kc=src[srcp++];
    int vc=(src[srcp]<<8)|src[srcp+1];
    srcp+=2;
    if (srcp>srcc-vc-kc) break;
    const char *k=(char*)(src+srcp); srcp+=kc;
    const char *v=(char*)(src+srcp); srcp+=vc;
    sr_encode_json_string(dst,k,kc,v,vc);
  }
  sr_encode_json_end(dst,bodyctx);
  
  return sr_encode_json_end(dst,outerctx);
}

/* Encode and queue event.
 * I've fought the temptation to encode it directly in (wbuf).
 * For cleanliness's sake, encode the JSON separately then copy in full.
 */
 
int output_queue_event_loose(struct output *output,const char *path,const void *src,int srcc) {
  if (!output||!path) return -1;
  struct sr_encoder json={0};
  if (output_encode_event(&json,path,src,srcc)<0) {
    sr_encoder_cleanup(&json);
    return -1;
  }
  int err=output_queue_event(output,json.v,json.c);
  sr_encoder_cleanup(&json);
  return err;
}
