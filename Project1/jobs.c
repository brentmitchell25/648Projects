#include "jobs.h"
#include <sys/queue.h>
#include <stdlib.h>



void add_job(struct Main *m, const char *command, int job_id, int pid)
{
   struct Jobs *j = malloc(sizeof *j);
   strcpy(j->command,command);
   j->job_id = job_id;
   j->pid = pid;
   TAILQ_INSERT_TAIL(&m->head, j, jobs);
}

void print_jobs(struct Main *m)
{
    struct Jobs *j;
    printf("%s\t%d\n", m->main, m->pid);
    TAILQ_FOREACH(j, &m->head, jobs) {
        printf("[%d]\t%d\t%s\n", j->job_id,j->pid,j->command);
    }
}
int delete_job(struct Main *m, int pid)
{
    struct Jobs *j, *next;
    for(j = TAILQ_FIRST(&m->head) ; j != NULL ; j = next) {
        next = TAILQ_NEXT(j, jobs);
        if(j->pid == pid) {
        	printf("[%d]\t%d\tfinished\t%s\n",j->job_id,j->pid,j->command);
            TAILQ_REMOVE(&m->head, j, jobs);
            free(j);
            return 1;
        }
    }
    return 0;

}
