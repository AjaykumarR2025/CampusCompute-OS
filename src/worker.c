#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/worker.h"

void worker_run(int simulate_failure)
{
    pid_t pid;
    int status;

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return;
    }

    if (pid == 0)
    {
        printf("Child process started. PID: %d\n", getpid());

        if (simulate_failure)
        {
            printf("Simulating controlled worker failure.\n");
            fflush(stdout);
            exit(1);
        }

        printf("Worker completed its task normally.\n");
        fflush(stdout);
        exit(0);
    }
    else
    {
        printf("Parent waiting for child PID: %d\n", pid);

        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid");
            return;
        }

        if (WIFEXITED(status))
        {
            int exit_status = WEXITSTATUS(status);

            if (exit_status == 0)
            {
                printf("Worker %d completed normally. Exit status: %d\n",
                       pid, exit_status);
            }
            else
            {
                printf("Worker %d failed. Exit status: %d\n",
                       pid, exit_status);
            }
        }
        else if (WIFSIGNALED(status))
        {
            printf("Worker %d terminated by signal: %d\n",
                   pid, WTERMSIG(status));
        }
    }
}
