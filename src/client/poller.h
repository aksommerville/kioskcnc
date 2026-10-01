/* poller.h
 * Wraps poll() generically with some conveniences.
 * We don't own the open files, or even track them persistently.
 * Caller informs us of every interesting file during every update cycle.
 */
 
#ifndef POLLER_H
#define POLLER_H

struct poller;

void poller_del(struct poller *poller);
struct poller *poller_new();

/* Forget any registered files.
 * Do this at the start of the update cycle.
 * Shouldn't be necessary but it's an easy safety net.
 */
void poller_flush(struct poller *poller);

/* Register your interest in a file, we'll poll it at the next poller_update().
 * The plain version just forwards notifications from poll(), no additional I/O.
 * _buffered() reads for you into a temporary buffer. Empty on errors or EOF.
 * If you need to unregister a file, call either of these with both callbacks null.
 * Only one registration is allowed per fd per cycle; we'll overwrite any existing one.
 */
int poller_register_file(
  struct poller *poller,
  int fd,void *userdata,
  int (*cb_readable)(int fd,void *userdata),
  int (*cb_writeable)(int fd,void *userdata)
);
int poller_register_file_buffered(
  struct poller *poller,
  int fd,void *userdata,
  int (*cb_read)(int fd,void *userdata,const void *src,int srcc),
  int (*cb_writeable)(int fd,void *userdata)
);

/* Do the poll. Sleep and fire callbacks as warranted.
 * If no files are listening, we sleep for the given timeout.
 */
int poller_update(struct poller *poller,int toms);

#endif
