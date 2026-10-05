/* foutput.h
 * Alternative to the networked "output" unit.
 * "foutput" exposes a similar interface, but just dumps regular files into a local directory.
 */
 
#ifndef FOUTPUT_H
#define FOUTPUT_H

struct foutput;

void foutput_del(struct foutput *foutput);
struct foutput *foutput_new();

/* Exactly equivalent to output_queue_event_loose() but it goes somewhere else.
 * We say "queue" for consistency with that existing interface, but in fact it writes out synchronously.
 */
int foutput_queue_event_loose(struct foutput *foutput,const char *path,const void *src,int srcc);

#endif
