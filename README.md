# CampusCompute – Member 2 Submission Files

Student SRN: PES1UG25CS043

This folder contains the Member 2 implementation for the CampusCompute Operating Systems mini-project.

## Files included

- `member2.c` — C implementation of FR3, FR4, FR7 and FR8.
- `Makefile` — build instructions.
- `README.md` — prerequisites, commands, testing and cleanup.
- `requirements.txt` — required Linux system packages/APIs.

## Requirements implemented

- **FR3:** POSIX producer-consumer logger using pthreads, a bounded queue, mutex and condition variables. Consumer events are appended to `campuscompute.log`.
- **FR4:** Parent/child communication using POSIX shared memory and a named semaphore. The program waits for the child and unlinks the IPC resources it creates.
- **FR7:** Demonstrates `setrlimit()` with `RLIMIT_NOFILE`; also joins threads, waits for the child, and releases created resources.
- **FR8:** CLI commands, argument validation and error/usage messages.

## Build on Ubuntu / WSL Ubuntu

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

View generated logs and clean generated files:

```bash
cat campuscompute.log
make clean
```

## Final group-submission note

This ZIP contains the Member 2 contribution only. Add Member 1's `.c` and `.h` source files to the shared project, reconcile the common build/CLI, then have Member 1 verify the complete source package as your division-of-work table specifies.

The required `PES1UG25CS043.pdf` evidence PDF is **not included as completed evidence**. It must contain genuine screenshots captured from your own terminal by both members, with the full SRN visible in the terminal prompt and a one-line explanation under each screenshot. Do not submit fabricated or blank screenshots as evidence.
