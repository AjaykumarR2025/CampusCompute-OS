# CampusCompute OS

CampusCompute is a C-based command-line prototype for the Operating Systems mini-project (UE25CS242A, Level 3). This README contains the documentation of both team members.

| Member | SRN | Part |
|---|---|---|
| Member 1 | PES1UG25CS041 | [Part 1 – Member 1](#part-1--member-1-pes1ug25cs041) |
| Member 2 | PES1UG25CS043 | [Part 2 – Member 2](#part-2--member-2-pes1ug25cs043) |

An integrated build and run guide for the complete program is in [Part 3](#part-3--integrated-build-and-run).

---

## Part 1 – Member 1 (PES1UG25CS041)

CampusCompute OS is a C-based operating-system simulation project developed as part of an academic team project. It demonstrates key operating-system concepts through command-line programs.

> **Note:** This project is a simulator for learning and demonstration. It is not a replacement for a real operating system.

### Features

#### 1. CPU Scheduling

- First Come, First Served (FCFS)
- Round Robin (RR)
- Priority Scheduling
- Scheduling comparison

#### 2. Process and Resource Management

- Banker's Algorithm for deadlock avoidance
- Safe and unsafe state demonstrations
- Simulated worker processes
- Worker failure and resource-limit demonstrations

#### 3. Concurrency and Inter-Process Communication

- Concurrent activity logging
- Thread synchronization and termination
- Shared memory communication
- Semaphore-based coordination and cleanup

### Technologies Used

- C
- GCC
- Make
- POSIX threads (`pthread`)
- Shared memory and semaphores
- Ubuntu/Linux

### Project Structure

The project is organized into source files, headers, and test or demonstration programs.

```text
CampusCompute-OS/
├── main.c        # Command-line interface and menu
├── src/          # Implementation source files
├── include/      # Header files
├── Makefile      # Build instructions
├── README.md     # Project documentation
└── .gitignore    # Files excluded from Git
```

### Build and Run

#### Prerequisites

- Ubuntu or another Linux environment
- GCC compiler
- GNU Make
- POSIX thread support

Check the installed tools:

```bash
gcc --version
make --version
```

Build the project:

```bash
make
```

Run the executable using the command supported by your project:

```bash
./campuscompute
```

To remove generated build files:

```bash
make clean
```

---

## Part 2 – Member 2 (PES1UG25CS043)

### CampusCompute – Member 2 Submission Files

Student SRN: PES1UG25CS043

This folder contains the Member 2 implementation for the CampusCompute Operating Systems mini-project.

### Files included

- `member2.c` — C implementation of FR3, FR4, FR7 and FR8.
- `Makefile` — build instructions.
- `README.md` — prerequisites, commands, testing and cleanup.
- `requirements.txt` — required Linux system packages/APIs.

### Requirements implemented

- **FR3:** POSIX producer-consumer logger using pthreads, a bounded queue, mutex and condition variables. Consumer events are appended to `campuscompute.log`.
- **FR4:** Parent/child communication using POSIX shared memory and a named semaphore. The program waits for the child and unlinks the IPC resources it creates.
- **FR7:** Demonstrates `setrlimit()` with `RLIMIT_NOFILE`; also joins threads, waits for the child, and releases created resources.
- **FR8:** CLI commands, argument validation and error/usage messages.

### Build on Ubuntu / WSL Ubuntu

Install the compiler and Make:

```bash
sudo apt update
sudo apt install build-essential
```

Build:

```bash
make
```

Run each feature:

```bash
./campuscompute_member2 help
./campuscompute_member2 logger 10
./campuscompute_member2 ipc
./campuscompute_member2 limits
./campuscompute_member2 all
```

> **Integrated project:** in the combined project the same commands are run through the single `campuscompute` executable, for example `./campuscompute logger 10`, `./campuscompute ipc` and `./campuscompute limits`. See [Part 3](#part-3--integrated-build-and-run).

View generated logs and clean generated files:

```bash
cat campuscompute.log
make clean
```

### Final group-submission note

This ZIP contains the Member 2 contribution only. Add Member 1's `.c` and `.h` source files to the shared project, reconcile the common build/CLI, then have Member 1 verify the complete source package as your division-of-work table specifies.

The required `PES1UG25CS043.pdf` evidence PDF is **not included as completed evidence**. It must contain genuine screenshots captured from your own terminal by both members, with the full SRN visible in the terminal prompt and a one-line explanation under each screenshot. Do not submit fabricated or blank screenshots as evidence.

---

## Part 3 – Integrated Build and Run

The complete program joins both members' modules behind one executable, `campuscompute`.

### Source layout

```text
CampusCompute-OS/
├── main.c              # Command-line interface and menu
├── Makefile
├── README.md
├── include/
│   ├── jobs.h
│   ├── scheduling.h
│   ├── bankers.h
│   ├── worker.h
│   └── member2.h       # logger, IPC and resource-limit entry point
└── src/
    ├── jobs.c
    ├── scheduling.c
    ├── bankers.c
    ├── worker.c
    └── member2.c
```

### Build

```bash
sudo apt update && sudo apt install build-essential
make
```

### Run

Running the program with no command opens a numbered menu (1 to 14, with 0 to exit):

```bash
./campuscompute
```

Every menu item is also available as a command:

```bash
./campuscompute help            # list all commands
./campuscompute jobs            # job lifecycle: Waiting -> Running -> Completed
./campuscompute fcfs            # First Come, First Served
./campuscompute rr 2            # Round Robin with time quantum 2
./campuscompute priority        # non-preemptive Priority
./campuscompute compare         # all three policies on the same job set
./campuscompute bankers-safe    # Banker's algorithm, safe state
./campuscompute bankers-unsafe  # Banker's algorithm, unsafe state
./campuscompute worker-normal   # child completes normally
./campuscompute worker-fail     # SIMULATED child failure
./campuscompute logger 10       # producer-consumer logger, 10 events
./campuscompute ipc             # shared memory + named semaphore
./campuscompute limits          # RLIMIT_NOFILE demonstration
./campuscompute all             # every demonstration, then a summary
```

By default the job and scheduling commands use a built-in sample set (ID, arrival, burst, priority): `1 0 5 2`, `2 1 3 1`, `3 2 6 3`. Add `-i` to `jobs`, `fcfs`, `rr`, `priority` or `compare` to enter your own jobs.

### Tie-breaking rules

- **FCFS:** earlier arrival first, then lower Job ID.
- **Round Robin:** arrival order, then lower Job ID; a job that arrives while another is running is queued before the job that was just preempted.
- **Priority:** a lower priority number means higher priority; ties are broken by earlier arrival, then lower Job ID.

### Logs and cleanup

```bash
cat campuscompute.log
pgrep -a campuscompute || echo "no campuscompute processes running"
ls /dev/shm | grep campuscompute || echo "no campuscompute IPC objects left"
make clean        # removes only the executable and the log
```
