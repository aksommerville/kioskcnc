/* savewatch.h
 * Sets up inotify for known saved games, and dispatches on changes.
 */
 
#ifndef SAVEWATCH_H
#define SAVEWATCH_H

struct poller;
struct savewatch;

void savewatch_del(struct savewatch *savewatch);

struct savewatch *savewatch_new();

int savewatch_register_files(struct savewatch *savewatch,struct poller *poller);

/* During the poll, we only mark files as dirty and set a timeout.
 * Further dispatch is delayed until some tasteful interval elapsed, to allow noisy sources time to settle down.
 * Call this after each poll cycle.
 */
int savewatch_update(struct savewatch *savewatch);

#endif
