/* output.h
 * Handles transfer of changed save files to the remote server.
 */
 
#ifndef OUTPUT_H
#define OUTPUT_H

struct output;
struct poller;
struct sr_encoder;

void output_del(struct output *output);
struct output *output_new(const char *host_and_port);

int output_register_files(struct output *output,struct poller *poller);

int output_update(struct output *output);

/* Give us a JSON event ({host,file,body}).
 * We'll compose the HTTP request, queue to the output, and send at the next opportunity.
 */
int output_queue_event(struct output *output,const char *body,int bodyc);

/* More useful: Compose the JSON body and queue it.
 * (src) is a binary Egg saved game. We'll turn it into a JSON object.
 */
int output_queue_event_loose(struct output *output,const char *path,const void *src,int srcc);

/* Take a path, a binary Egg save file, and some globals.
 * Emits JSON text suitable for long-term capture.
 * Time is not encoded. We assume whoever writes the final file will sample it at that time and encode in the path.
 */
int output_encode_event(struct sr_encoder *dst,const char *path,const uint8_t *src,int srcc);

#endif
