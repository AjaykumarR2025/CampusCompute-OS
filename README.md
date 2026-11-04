# CampusCompute OS

CampusCompute OS is a C-based operating-system simulation project developed as part of an academic team project. It demonstrates key operating-system concepts through command-line programs.

> **Note:** This project is a simulator for learning and demonstration. It is not a replacement for a real operating system.

## Features

### 1. CPU Scheduling

- First Come, First Served (FCFS)
- Round Robin (RR)
- Priority Scheduling
- Scheduling comparison

### 2. Process and Resource Management

- Banker's Algorithm for deadlock avoidance
- Safe and unsafe state demonstrations
- Simulated worker processes
- Worker failure and resource-limit demonstrations

### 3. Concurrency and Inter-Process Communication

- Concurrent activity logging
- Thread synchronization and termination
- Shared memory communication
- Semaphore-based coordination and cleanup

## Technologies Used

- C
- GCC
- Make
- POSIX threads (`pthread`)
- Shared memory and semaphores
- Ubuntu/Linux

## Project Structure

The project is organized into source files, headers, and test or demonstration programs.

```text
CampusCompute-OS/
├── src/          # Implementation source files
├── include/      # Header files
├── tests/        # Test and demonstration programs
├── Makefile      # Build instructions
├── README.md     # Project documentation
└── .gitignore    # Files excluded from Git
```

*Update this structure to match the actual folders in your repository.*

## Build and Run

### Prerequisites

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
./<executable-name>
```

To remove generated build files:

```bash
make clean
```

