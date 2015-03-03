#ifndef PROJECT1_JOBS_H_
#define PROJECT1_JOBS_H_
#include <sys/queue.h>
#define MAX_LINE		256 /* 256 chars per line, per command */

struct Jobs {
   char command[MAX_LINE];
   int job_id;
   int pid;

    //linked list entry
    TAILQ_ENTRY(Jobs) jobs;
};
struct Main {
   char* main[MAX_LINE];
   int pid;
   //list of blocks in the "file"
   TAILQ_HEAD(,Jobs) head;
};

void add_job(struct Main *m, const char *command, int job_id, int pid);
void print_jobs(struct Main *f);
int delete_job(struct Main *f, int pid);

#endif /* PROJECT1_JOBS_H_ */
