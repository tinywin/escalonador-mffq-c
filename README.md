
# Process Scheduling with Priority Queues

This repository contains a C language implementation of a process scheduling algorithm based on **Priority Queues with Round Robin**, developed as a practical individual assignment for the Operating Systems course.

## 📋 Project Overview

The objective of this assignment is to simulate process management and CPU scheduling within an Operating System using multiple circularly linked queues featuring preemption and dynamic priority feedback.

### Assignment Requirements
- **Programming Language**: C
- **Priority Queues**: At least 4 distinct priority levels (0 to 3, where 0 is the highest priority).
- **Queue Implementation**: Circular lists representing **Round Robin** scheduling for each level.
- **Process Structure**: Queue nodes consist of Process Control Block (**PCB**) structures.
- **Quantum**: Static quantum defined per queue level:
  - **Queue 0**: Quantum = 2
  - **Queue 1**: Quantum = 4
  - **Queue 2**: Quantum = 6
  - **Queue 3**: Quantum = 8

---

## 🛠️ Code Structure

- **PCB (`struct PCB`)**: Stores process attributes (PID, name, state, arrival time, burst time, remaining time, start/end times, priority, and next pointer).
- **Queue (`typedef struct Fila`)**: Manages the circular linked list and stores queue-specific metrics (quantum, current process pointer, priority).
- **System (`typedef struct Sistema`)**: Controls the global simulation clock, process admission, context switching, and statistical reporting.

---

## 🚀 How to Compile and Run

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/YOUR_USERNAME/YOUR_REPOSITORY_NAME.git](https://github.com/YOUR_USERNAME/YOUR_REPOSITORY_NAME.git)
   cd YOUR_REPOSITORY_NAME

 * Compile using GCC:
   gcc -Wall -Wextra escalonadr-lbh.c -o scheduler

 * Run the simulation:
   ./scheduler

---

📊 Output & Metrics
The simulation executes a set of workload processes (navegador, compilador, backup, player, etc.) and outputs:
 * Execution Log: Step-by-step tracing of process arrivals, dispatches, quantum expirations/demotions, and completions.
 * Timeline (Gantt Chart): Visual representation of CPU time segments.
 * Performance Metrics:
   * Turnaround Time
   * Waiting Time
   * Response Time
 * Global Metrics: Context switches, idle ticks, and calculated averages across all processes.