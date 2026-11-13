/*
 * CampusCompute - command-line front end.
 * Joins job management/scheduling (FR1, FR2), Banker's algorithm (FR5),
 * worker processes (FR6) and the logger / IPC / resource-limit modules
 * (FR3, FR4, FR7) behind one CLI (FR8).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "jobs.h"
#include "scheduling.h"
#include "bankers.h"
#include "worker.h"
#include "member2.h"

#define SAMPLE_JOBS 3
#define DEFAULT_QUANTUM 2

static void usage(const char *prog) {
    printf("CampusCompute - Intelligent Job Scheduling and Resource Management\n");
    printf("Usage: %s <command> [options]\n\n", prog);
    printf("Commands:\n");
    printf("  jobs [-i]            Job management: Waiting -> Running -> Completed\n");
    printf("  fcfs [-i]            First-Come First-Served scheduling\n");
    printf("  rr [quantum] [-i]    Round Robin scheduling (default quantum %d)\n", DEFAULT_QUANTUM);
    printf("  priority [-i]        Non-preemptive Priority scheduling\n");
    printf("  compare [-i]         FCFS, Round Robin and Priority on the same job set\n");
    printf("  bankers-safe         Banker's algorithm on a safe state\n");
    printf("  bankers-unsafe       Banker's algorithm on an unsafe state\n");
    printf("  worker-normal        Child process that completes normally\n");
    printf("  worker-fail          Child process with a SIMULATED failure\n");
    printf("  logger [count]       Producer-consumer logger (default 10 events)\n");
    printf("  ipc                  Shared memory + named semaphore between processes\n");
    printf("  limits               Resource limit demo (setrlimit, RLIMIT_NOFILE)\n");
    printf("  all                  Run every demonstration and print a summary\n");
    printf("  menu                 Interactive numbered menu (default when no command is given)\n");
    printf("  help                 Show this message\n\n");
    printf("-i means: enter the jobs yourself. Without it a built-in sample set is used:\n");
    printf("  (ID Arrival Burst Priority) = (1 0 5 2) (2 1 3 1) (3 2 6 3)\n");
}

static int bad_usage(const char *prog, const char *message) {
    fprintf(stderr, "Error: %s\n", message);
    usage(prog);
    return 2;
}

/* ---------- job sets ---------- */

static void load_sample_jobs(Job jobs[], int *n) {
    static const int sample[SAMPLE_JOBS][4] = {{1, 0, 5, 2}, {2, 1, 3, 1}, {3, 2, 6, 3}};
    for (int i = 0; i < SAMPLE_JOBS; i++) {
        jobs[i].id = sample[i][0];
        jobs[i].arrival = sample[i][1];
        jobs[i].burst = sample[i][2];
        jobs[i].priority = sample[i][3];
        jobs[i].start_time = -1;
        jobs[i].completion_time = -1;
        jobs[i].status = JOB_WAITING;
    }
    *n = SAMPLE_JOBS;
}

static int get_jobs(Job jobs[], int *n, int interactive) {
    if (interactive) {
        if (input_jobs(jobs, n) != 0) {
            fprintf(stderr, "Error: job input failed.\n");
            return -1;
        }
    } else {
        load_sample_jobs(jobs, n);
        printf("Using the built-in sample job set.\n");
    }
    return 0;
}

/* Parses the options after the command name. */
static int parse_options(int argc, char **argv, int allow_quantum,
                         int *interactive, int *quantum) {
    *interactive = 0;
    *quantum = DEFAULT_QUANTUM;
    int quantum_set = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            *interactive = 1;
        } else if (allow_quantum && !quantum_set) {
            char *end;
            errno = 0;
            long q = strtol(argv[i], &end, 10);
            if (errno != 0 || end == argv[i] || *end != '\0' || q < 1 || q > 100)
                return -1;
            *quantum = (int)q;
            quantum_set = 1;
        } else {
            return -1;
        }
    }
    return 0;
}

static void banner(const char *title) {
    printf("\n========== %s ==========\n", title);
}

/* ---------- FR1 / FR2 ---------- */

static int cmd_jobs(int interactive) {
    Job jobs[MAX_JOBS];
    int n = 0;
    if (get_jobs(jobs, &n, interactive) != 0) return 1;
    print_jobs(jobs, n);
    run_lifecycle(jobs, n);
    print_job_results(jobs, n);
    return 0;
}

/* which: 0 = fcfs, 1 = rr, 2 = priority, 3 = all three */
static int cmd_schedule(int which, int quantum, int interactive) {
    Job jobs[MAX_JOBS];
    int n = 0;
    if (get_jobs(jobs, &n, interactive) != 0) return 1;
    print_jobs(jobs, n);

    if (which == 0 || which == 3) {
        if (which == 3) banner("FCFS");
        fcfs_schedule(jobs, n);
    }
    if (which == 1 || which == 3) {
        if (which == 3) banner("ROUND ROBIN");
        round_robin_schedule(jobs, n, quantum);
    }
    if (which == 2 || which == 3) {
        if (which == 3) banner("PRIORITY");
        priority_schedule(jobs, n);
    }
    if (which == 3) banner("COMPARISON COMPLETE");
    return 0;
}

/* ---------- FR5 ---------- */

static void print_matrix(const char *name, int rows, int cols, int m[MAX_PROCESSES][MAX_RESOURCES]) {
    printf("\n%s\nProcess\t", name);
    for (int j = 0; j < cols; j++) printf("%c\t", 'A' + j);
    printf("\n");
    for (int i = 0; i < rows; i++) {
        printf("P%d\t", i);
        for (int j = 0; j < cols; j++) printf("%d\t", m[i][j]);
        printf("\n");
    }
}

/* Returns bankers_safety()'s result: 1 = safe, 0 = unsafe. */
static int cmd_bankers(int want_safe) {
    int n = 5, m = 3;
    int available[MAX_RESOURCES] = {0};
    int allocation[MAX_PROCESSES][MAX_RESOURCES] = {
        {0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};
    int maximum[MAX_PROCESSES][MAX_RESOURCES] = {
        {7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};

    if (want_safe) { available[0] = 3; available[1] = 3; available[2] = 2; }
    else           { available[0] = 1; available[1] = 0; available[2] = 0; }

    printf("Banker's test: %s state\n", want_safe ? "SAFE" : "UNSAFE");
    printf("\nAvailable: A=%d B=%d C=%d\n", available[0], available[1], available[2]);
    print_matrix("Allocation Matrix", n, m, allocation);
    print_matrix("Maximum Matrix", n, m, maximum);
    return bankers_safety(n, m, available, allocation, maximum);
}

/* ---------- FR6 ---------- */

static void cmd_worker(int simulate_failure) {
    if (simulate_failure)
        printf("NOTE: this failure is SIMULATED (the child calls exit(1) on purpose); it is not a real OS failure.\n");
    else
        printf("Starting a worker that will complete normally.\n");
    fflush(stdout);              /* avoid duplicated output in the forked child */
    worker_run(simulate_failure);
}

/* ---------- FR3 / FR4 / FR7 (logger, ipc, limits) ---------- */

static int run_member2(const char *prog, const char *cmd, const char *arg) {
    char *av[4];
    int ac = 0;
    av[ac++] = (char *)prog;
    av[ac++] = (char *)cmd;
    if (arg) av[ac++] = (char *)arg;
    av[ac] = NULL;
    fflush(stdout);
    return member2_main(ac, av);
}

/* ---------- integrated run ---------- */

static void summary_line(const char *what, const char *result) {
    printf("  %-44s %s\n", what, result);
}

static int cmd_all(const char *prog) {
    int failures = 0;
    int r_jobs, r_sched, r_safe, r_unsafe, r_log, r_ipc, r_lim;
    int safe_result, unsafe_result;

    banner("FR1 JOB LIFECYCLE");
    r_jobs = cmd_jobs(0);
    banner("FR2 SCHEDULING COMPARISON");
    r_sched = cmd_schedule(3, DEFAULT_QUANTUM, 0);

    banner("FR5 BANKER'S - SAFE STATE");
    safe_result = cmd_bankers(1);
    r_safe = (safe_result == 1) ? 0 : 1;           /* expected: safe */
    banner("FR5 BANKER'S - UNSAFE STATE");
    unsafe_result = cmd_bankers(0);
    r_unsafe = (unsafe_result == 0) ? 0 : 1;       /* expected: unsafe */

    banner("FR6 WORKER - NORMAL");
    cmd_worker(0);
    banner("FR6 WORKER - SIMULATED FAILURE");
    cmd_worker(1);

    banner("FR3 LOGGER");
    r_log = run_member2(prog, "logger", "8");
    banner("FR4 SHARED MEMORY + SEMAPHORE");
    r_ipc = run_member2(prog, "ipc", NULL);
    banner("FR7 RESOURCE LIMIT");
    r_lim = run_member2(prog, "limits", NULL);

    banner("INTEGRATED SUMMARY");
    summary_line("FR1 job lifecycle", r_jobs == 0 ? "OK" : "FAILED");
    summary_line("FR2 FCFS / Round Robin / Priority", r_sched == 0 ? "OK" : "FAILED");
    summary_line("FR5 Banker's safe state (expected SAFE)", r_safe == 0 ? "OK" : "FAILED");
    summary_line("FR5 Banker's unsafe state (expected UNSAFE)", r_unsafe == 0 ? "OK" : "FAILED");
    summary_line("FR6 worker normal + simulated failure", "OK (see output above)");
    summary_line("FR3 producer-consumer logger", r_log == 0 ? "OK" : "FAILED");
    summary_line("FR4 shared memory + semaphore", r_ipc == 0 ? "OK" : "FAILED");
    summary_line("FR7 resource limit + cleanup", r_lim == 0 ? "OK" : "FAILED");

    failures = r_jobs + r_sched + r_safe + r_unsafe + r_log + r_ipc + r_lim;
    printf("\nResult: %s\n", failures == 0 ? "all demonstrations completed" : "some demonstrations FAILED");
    return failures == 0 ? 0 : 1;
}

/* ---------- main ---------- */

static int dispatch(int argc, char **argv) {
    const char *prog = argv[0];
    int interactive, quantum;

    if (argc < 2) {
        usage(prog);
        return 1;
    }
    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0) {
        usage(prog);
        return 0;
    }
    if (strcmp(cmd, "jobs") == 0 || strcmp(cmd, "fcfs") == 0 ||
        strcmp(cmd, "priority") == 0 || strcmp(cmd, "compare") == 0 ||
        strcmp(cmd, "rr") == 0) {
        int is_rr = strcmp(cmd, "rr") == 0;
        if (parse_options(argc, argv, is_rr, &interactive, &quantum) != 0)
            return bad_usage(prog, is_rr ? "rr takes an optional quantum (1-100) and -i."
                                         : "this command accepts only the -i option.");
        if (strcmp(cmd, "jobs") == 0)     return cmd_jobs(interactive);
        if (strcmp(cmd, "fcfs") == 0)     return cmd_schedule(0, quantum, interactive);
        if (is_rr)                        return cmd_schedule(1, quantum, interactive);
        if (strcmp(cmd, "priority") == 0) return cmd_schedule(2, quantum, interactive);
        return cmd_schedule(3, quantum, interactive);
    }
    if (strcmp(cmd, "bankers-safe") == 0 || strcmp(cmd, "bankers-unsafe") == 0 ||
        strcmp(cmd, "worker-normal") == 0 || strcmp(cmd, "worker-fail") == 0 ||
        strcmp(cmd, "ipc") == 0 || strcmp(cmd, "limits") == 0 ||
        strcmp(cmd, "all") == 0) {
        if (argc != 2)
            return bad_usage(prog, "this command takes no arguments.");
        if (strcmp(cmd, "bankers-safe") == 0)   { cmd_bankers(1); return 0; }
        if (strcmp(cmd, "bankers-unsafe") == 0) { cmd_bankers(0); return 0; }
        if (strcmp(cmd, "worker-normal") == 0)  { cmd_worker(0); return 0; }
        if (strcmp(cmd, "worker-fail") == 0)    { cmd_worker(1); return 0; }
        if (strcmp(cmd, "all") == 0)            return cmd_all(prog);
        return member2_main(argc, argv);        /* ipc, limits */
    }
    if (strcmp(cmd, "logger") == 0) {
        if (argc > 3)
            return bad_usage(prog, "logger takes at most one argument (event count).");
        if (argc == 3) {
            char *end;
            errno = 0;
            long c = strtol(argv[2], &end, 10);
            if (errno != 0 || end == argv[2] || *end != '\0' || c < 1 || c > 1000)
                return bad_usage(prog, "logger count must be an integer from 1 to 1000.");
        }
        return member2_main(argc, argv);
    }

    char msg[128];
    snprintf(msg, sizeof msg, "unknown command '%s'.", cmd);
    return bad_usage(prog, msg);
}

/* ---------- interactive menu ---------- */

/* Reads one line; returns 0 on success, -1 on EOF. Strips the newline. */
static int read_line(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)size, stdin)) return -1;
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

/* Reads an integer in [min, max]; empty input returns def. Re-prompts on bad input. */
static int read_number(const char *prompt, int min, int max, int def, int *out) {
    char buf[64];
    for (;;) {
        if (read_line(prompt, buf, sizeof buf) != 0) return -1;
        if (buf[0] == '\0') { *out = def; return 0; }
        char *end;
        errno = 0;
        long v = strtol(buf, &end, 10);
        if (errno == 0 && end != buf && *end == '\0' && v >= min && v <= max) {
            *out = (int)v;
            return 0;
        }
        printf("Invalid input: enter a whole number from %d to %d.\n", min, max);
    }
}

static void print_menu(void) {
    printf("\n=========== CampusCompute Menu ===========\n");
    printf("  1.  Job management (Waiting -> Running -> Completed)\n");
    printf("  2.  FCFS scheduling\n");
    printf("  3.  Round Robin scheduling\n");
    printf("  4.  Priority scheduling\n");
    printf("  5.  Compare all scheduling policies\n");
    printf("  6.  Banker's algorithm - safe state\n");
    printf("  7.  Banker's algorithm - unsafe state\n");
    printf("  8.  Worker process - normal completion\n");
    printf("  9.  Worker process - simulated failure\n");
    printf("  10. Producer-consumer logger\n");
    printf("  11. Shared memory + semaphore IPC\n");
    printf("  12. Resource limit demo\n");
    printf("  13. Run everything (integrated demo)\n");
    printf("  14. Show command-line usage\n");
    printf("  0.  Exit\n");
    printf("==========================================\n");
}

/* Asks whether to use the sample jobs or enter them; returns 1 for -i, 0 for sample, -1 on EOF. */
static int ask_job_source(void) {
    int choice;
    printf("\nJob set: 1 = built-in sample, 2 = enter my own\n");
    if (read_number("Choose (default 1): ", 1, 2, 1, &choice) != 0) return -1;
    return choice == 2;
}

static int menu_loop(const char *prog) {
    static const char *const simple[] = {
        NULL, NULL, NULL, NULL, NULL, NULL,
        "bankers-safe", "bankers-unsafe", "worker-normal", "worker-fail"
    };
    for (;;) {
        char buf[64];
        print_menu();
        if (read_line("Enter your choice (0-14): ", buf, sizeof buf) != 0) {
            printf("\n");
            return 0;                              /* EOF: leave quietly */
        }
        char *end;
        errno = 0;
        long c = strtol(buf, &end, 10);
        if (buf[0] == '\0' || errno != 0 || end == buf || *end != '\0' || c < 0 || c > 14) {
            printf("Invalid choice '%s'. Enter a number from 0 to 14.\n", buf);
            continue;
        }
        if (c == 0) {
            printf("Goodbye.\n");
            return 0;
        }

        char *av[5] = { (char *)prog, NULL, NULL, NULL, NULL };
        char qbuf[16], cbuf[16];
        int ac = 2, src;

        if (c >= 6 && c <= 9) {
            av[1] = (char *)simple[c];
        } else if (c >= 1 && c <= 5) {
            static const char *const names[] = { NULL, "jobs", "fcfs", "rr", "priority", "compare" };
            av[1] = (char *)names[c];
            if (c == 3) {
                int q;
                if (read_number("Time quantum (1-100, default 2): ", 1, 100, DEFAULT_QUANTUM, &q) != 0) return 0;
                snprintf(qbuf, sizeof qbuf, "%d", q);
                av[ac++] = qbuf;
            }
            src = ask_job_source();
            if (src < 0) return 0;
            if (src == 1) av[ac++] = (char *)"-i";
        } else if (c == 10) {
            int count;
            if (read_number("Number of events (1-1000, default 10): ", 1, 1000, 10, &count) != 0) return 0;
            snprintf(cbuf, sizeof cbuf, "%d", count);
            av[1] = (char *)"logger";
            av[ac++] = cbuf;
        } else if (c == 11) {
            av[1] = (char *)"ipc";
        } else if (c == 12) {
            av[1] = (char *)"limits";
        } else if (c == 13) {
            av[1] = (char *)"all";
        } else {
            av[1] = (char *)"help";
        }
        av[ac] = NULL;
        fflush(stdout);
        dispatch(ac, av);
    }
}

int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "menu") == 0))
        return menu_loop(argv[0]);
    return dispatch(argc, argv);
}
