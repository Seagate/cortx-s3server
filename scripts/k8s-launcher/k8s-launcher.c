
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define MAX_JOBS 256
#define MAX_CMD_LEN 4096

int jobs_count;
struct {
    char *cmd_line;
    pid_t pid;
    long last_restart_timestamp;
} jobs[MAX_JOBS];

void proc_exit() {
    union wait wstat;
    pid_t pid;

    while (1) {
        pid = waitpid(-1, &wstat, WNOHANG);
        if (pid == 0)
            return;
        else if (pid == -1)
            return;
        else { /* Child with pid = 'pid' changed state */
            /*     STOPPED HERE   */
            /* TBD: */
            1. iterate over jobs and find jobs[i].pid==pid
                2. try restart
        }
    }
}

int main(int argc, char **argv) {
    char *jobs_file_name; /* Name of file with jobs to launch; one cmdline per line. */
    int i;

    /* Parse args */
    if (argc != 2) {
        printf("Usage: %s jobs_file\n", argv[0]);
        exit(1);
    }
    jobs_file_name = argv[1];

    /* Load jobs */
    {
        FILE *f = fopen(jobs_file_name, "r");
        char file_entry[MAX_CMD_LEN + 3]; /* 1 char for \n, 1 char for \0,
                                           * 1 for EOF vs EOL check */

        jobs_count = 0;

        if (!f) {
            perror("Loading jobs failed");
            exit(1);
        }

        while (fgets(file_entry, sizeof(file_entry), f)) {
            int l = strlen(file_entry);

            if (jobs_count >= MAX_JOBS) {
                printf("Too many jobs specified (more than max=%d)\n", MAX_JOBS);
                exit(1);
            }
            if (l <= 0) {
                continue;
            }
            if (file_entry[l - 1] == '\n') {
                file_entry[l - 1] = '\0';
                l--;
            }
            if (l == 0) {
                continue;
            }
            if (l > MAX_CMD_LEN) {
                printf("Too long command specified (%d chars at %d-th cmd)\n", l, jobs_count + 1);
                exit(1);
            }
            bzero(&jobs[jobs_count], sizeof(jobs[0]));
            jobs[jobs_count].cmd_line = strdup(file_entry);
            jobs_count++;
        }
    }

    /* Run */
    signal(SIGCHLD, proc_exit);
    for (i = 0; i < jobs_count; i++) {
        int pid = fork();

        if (pid == -1 ) {
            perror("Failed to fork()");
            exit(1);
        } else if (pid == 0) { /* Parent */
            jobs[i].pid = pid;
        } else { /* Child */
            (void)system(jobs[i].cmd_line);
            exit(0);
        }
    }

    while (1) {
        pause();
    }
}
