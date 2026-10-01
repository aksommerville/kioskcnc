#include "kioskcnc.h"
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>

struct poller {
  struct pollfd *pollfdv;
  int pollfdc,pollfda;
  struct manifold {
    int fd; // Should be listed in (pollfdv).
    void *userdata;
    int (*cb_readable)(int fd,void *userdata);
    int (*cb_writeable)(int fd,void *userdata);
    int (*cb_read)(int fd,void *userdata,const void *src,int srcc);
  } *manifoldv;
  int manifoldc,manifolda;
  char *buf;
  int bufa;
  int lock; // Nonzero during updates so we can reject structural changes.
};

/* Delete.
 */
 
void poller_del(struct poller *poller) {
  if (!poller) return;
  if (poller->pollfdv) free(poller->pollfdv);
  if (poller->manifoldv) free(poller->manifoldv);
  if (poller->buf) free(poller->buf);
  free(poller);
}

/* New.
 */
 
struct poller *poller_new() {
  struct poller *poller=calloc(1,sizeof(struct poller));
  if (!poller) return 0;
  return poller;
}

/* Flush files.
 */

void poller_flush(struct poller *poller) {
  if (!poller) return;
  poller->pollfdc=0;
  poller->manifoldc=0;
}

/* Find manifold by fd.
 */
 
static struct manifold *poller_find_manifold(struct poller *poller,int fd) {
  struct manifold *m=poller->manifoldv;
  int i=poller->manifoldc;
  for (;i-->0;m++) if (m->fd==fd) return m;
  return 0;
}

/* Add manifold to the end of our list.
 * Caller must ensure we don't have this fd already.
 */
 
static struct manifold *poller_add_manifold(struct poller *poller,int fd) {
  if (poller->manifoldc>=poller->manifolda) {
    int na=poller->manifolda+16;
    if (na>INT_MAX/sizeof(struct manifold)) return 0;
    void *nv=realloc(poller->manifoldv,sizeof(struct manifold)*na);
    if (!nv) return 0;
    poller->manifoldv=nv;
    poller->manifolda=na;
  }
  struct manifold *m=poller->manifoldv+poller->manifoldc++;
  memset(m,0,sizeof(struct manifold));
  m->fd=fd;
  return m;
}

/* Ensure we have a pollfd for the given manifold, creating if necessary.
 * You must call this after changing the manifold, so we can set the correct event bits.
 */
 
static int poller_require_pollfd(struct poller *poller,const struct manifold *m) {
  struct pollfd *pollfd=0;
  struct pollfd *q=poller->pollfdv;
  int i=poller->pollfdc;
  for (;i-->0;q++) {
    if (q->fd==m->fd) {
      pollfd=q;
      break;
    }
  }
  if (!pollfd) {
    if (poller->pollfdc>=poller->pollfda) {
      int na=poller->pollfda+16;
      if (na>INT_MAX/sizeof(struct pollfd)) return -1;
      void *nv=realloc(poller->pollfdv,sizeof(struct pollfd)*na);
      if (!nv) return -1;
      poller->pollfdv=nv;
      poller->pollfda=na;
    }
    pollfd=poller->pollfdv+poller->pollfdc++;
    memset(pollfd,0,sizeof(struct pollfd));
    pollfd->fd=m->fd;
  }
  pollfd->events=0;
  if (m->cb_readable||m->cb_read) {
    pollfd->events|=POLLIN|POLLERR|POLLHUP;
  }
  if (m->cb_writeable) {
    pollfd->events|=POLLOUT;
  }
  return 0;
}

/* Drop a pollfd.
 */
 
static void poller_remove_pollfd(struct poller *poller,int fd) {
  int i=poller->pollfdc;
  struct pollfd *pollfd=poller->pollfdv+i-1;
  for (;i-->0;pollfd--) {
    if (pollfd->fd==fd) {
      poller->pollfdc--;
      memmove(pollfd,pollfd+1,sizeof(struct pollfd)*(poller->pollfdc-i));
    }
  }
}

/* Register a file.
 */
 
static int poller_register_file_internal(
  struct poller *poller,
  int fd,void *userdata,
  int (*cb_readable)(int fd,void *userdata),
  int (*cb_writeable)(int fd,void *userdata),
  int (*cb_read)(int fd,void *userdata,const void *src,int srcc)
) {
  if (!poller||(fd<0)) return -1;
  
  /* If all three callbacks are null, unregister or noop.
   */
  if (!cb_readable&&!cb_writeable&&!cb_read) {
    int i=poller->manifoldc;
    struct manifold *m=poller->manifoldv+i-1;
    for (;i-->0;m--) {
      if (m->fd==fd) {
        poller->manifoldc--;
        memmove(m,m+1,sizeof(struct manifold)*(poller->manifoldc-i));
        return 0;
      }
    }
    poller_remove_pollfd(poller,fd);
    return 0;
  }
  
  /* If we already have it, replace userdata and callbacks in place.
   * Otherwise, add it.
   */
  int isnew=0;
  struct manifold *m=poller_find_manifold(poller,fd);
  if (!m) {
    if (!(m=poller_add_manifold(poller,fd))) return -1;
    isnew=1;
  }
  m->userdata=userdata;
  m->cb_readable=cb_readable;
  m->cb_writeable=cb_writeable;
  m->cb_read=cb_read;
  if (poller_require_pollfd(poller,m)<0) {
    if (isnew) poller->manifoldc--;
    return -1;
  }
  return 0;
}
 
int poller_register_file(
  struct poller *poller,
  int fd,void *userdata,
  int (*cb_readable)(int fd,void *userdata),
  int (*cb_writeable)(int fd,void *userdata)
) {
  return poller_register_file_internal(poller,fd,userdata,cb_readable,cb_writeable,0);
}

int poller_register_file_buffered(
  struct poller *poller,
  int fd,void *userdata,
  int (*cb_read)(int fd,void *userdata,const void *src,int srcc),
  int (*cb_writeable)(int fd,void *userdata)
) {
  return poller_register_file_internal(poller,fd,userdata,0,cb_writeable,cb_read);
}

/* Grow our buffer if needed and read into it.
 * Returns zero if the read fails, <0 only if allocation fails.
 */
 
static int poller_read_to_buffer(struct poller *poller,int fd) {
  const int reqc=4096; // Arbitrary minimum buffer size. As currently implemented, this will always be exactly the buffer size.
  if (reqc>poller->bufa) {
    void *nv=realloc(poller->buf,reqc);
    if (!nv) return -1;
    poller->buf=nv;
    poller->bufa=reqc;
  }
  int err=read(fd,poller->buf,poller->bufa);
  if (err<=0) return 0;
  return err;
}

/* File is readable or faulty.
 */
 
static int poller_read(struct poller *poller,int fd) {
  struct manifold *m=poller_find_manifold(poller,fd);
  if (!m) return 0;
  int err=0;
  if (m->cb_read) {
    int bufc=poller_read_to_buffer(poller,fd);
    if (bufc<0) err=bufc;
    else err=m->cb_read(fd,m->userdata,poller->buf,bufc);
  } else if (m->cb_readable) {
    err=m->cb_readable(fd,m->userdata);
  }
  return err;
}

/* File is writeable.
 */
 
static int poller_write(struct poller *poller,int fd) {
  struct manifold *m=poller_find_manifold(poller,fd);
  if (!m) return 0;
  int err=0;
  if (m->cb_writeable) {
    err=m->cb_writeable(fd,m->userdata);
  }
  return err;
}

/* Update.
 */

int poller_update(struct poller *poller,int toms) {

  /* If we have nothing to do, try to sleep for the full requested timeout.
   */
  if (!poller||(poller->pollfdc<1)) {
    if (toms>0) usleep(toms*1000);
    return 0;
  }
  
  /* Poll.
   */
  poller->lock=1;
  int err=poll(poller->pollfdv,poller->pollfdc,toms);
  if ((err<0)&&(errno==EINTR)) err=0; // Main will notice the interrupt (should already have done). Just return.
  
  /* Trigger callbacks.
   */
  if (err>0) {
    struct pollfd *pollfd=poller->pollfdv;
    int i=poller->pollfdc;
    for (;i-->0;pollfd++) {
      if (pollfd->revents&(POLLIN|POLLERR|POLLHUP)) {
        if ((err=poller_read(poller,pollfd->fd))<0) goto _done_;
      } else if (pollfd->revents&POLLOUT) {
        if ((err=poller_write(poller,pollfd->fd))<0) goto _done_;
      }
    }
  }

  /* Wrap up.
   */
 _done_:;
  poller->pollfdc=0;
  poller->manifoldc=0;
  poller->lock=0;
  if (err<0) return -1;
  return 0;
}
