# Programming-2---Multi-Process-IPC-Worker-Pool
Programming Assignment: Signal Hunters

Emergency Beacon Analysis System
C++11 | Thread Pools | Atomics | thread_local | Synchronization
Overview

Scenario. After a major natural disaster, emergency-response teams deploy autonomous receivers throughout the affected region. These receivers continuously record short segments of radio-spectrum data. Some segments contain emergency locator beacons, while most contain noise, interference, or unrelated transmissions.
A central computer receives thousands of independent signal segments that must be analyzed as quickly as possible. Your task is to develop a multithreaded C++11 Emergency Beacon Analysis System that analyzes these signal segments concurrently.
The workload is intentionally irregular. Some segments may contain only a few hundred samples, while others may contain tens of thousands. Consequently, assigning a fixed portion of the input to each thread is likely to produce poor load balancing. Your program should instead treat each signal segment as an independent unit of work that can be processed by any available worker.
Learning Objectives

Design and implement a reusable thread pool.
Use worker threads to process dynamically scheduled jobs.
Apply mutexes and condition variables to shared structures.
Use atomic variables for frequently observed worker telemetry.
Use thread-local storage for worker-specific state.
Measure and explain multithreaded performance.
Terminate threads cleanly without data races or deadlock.
Input Data

The input file contains one signal segment per line. Each segment contains a unique identifier, a signal-detection threshold, and a sequence of integer signal-strength measurements.
S001 70 12 15 17 74 82 91 78 20 18 14 11
S002 60 5 8 7 6 9 11 8 7 5
S003 50 12 53 58 61 47 10 55 64 59 8 6
The first value is the segment identifier, the second is the detection threshold, and all remaining values are signal-strength measurements. Test data may contain substantially larger segments.
Required Signal Analysis

Each segment must be analyzed independently. For every segment, determine at least the following:
Segment ID
Number of samples
Minimum signal strength
Maximum signal strength
Average signal strength
Number of samples above the detection threshold
Longest consecutive run above the threshold
Each segment must also be classified. A reasonable classification scheme might use:
NO SIGNAL
POSSIBLE SIGNAL
STRONG SIGNAL
You may develop your own reasonable classification algorithm, but the README must briefly explain it.
The quality of the signal-detection algorithm is not the primary purpose of this assignment; the concurrency architecture is more important.
Irregular Computational Workload

Signal segments may differ dramatically in size. For example:
S101      500 samples
S102   25,000 samples
S103    1,200 samples
S104   80,000 samples
S105      750 samples
A design that permanently assigns a fixed range of records to each thread may perform poorly because one thread could receive significantly more work than another.
The system should dynamically distribute signal-analysis jobs among available workers.
Thread Pool Requirement

Your program must use a thread pool.
The number of worker threads must be specified on the command line.
./signalHunter signals.txt 4
The example above processes signals.txt using four worker threads.
The program must not create a new thread for every signal segment. Worker threads should remain alive and process multiple jobs during execution.
When a worker finishes one segment, it should obtain another available segment until the workload has been exhausted.
How you organize the work queue, worker objects, synchronization objects, and thread pool is a design decision.
Worker Telemetry and Atomic State

Emergency-response supervisors want to observe the system while it is running. Each worker must therefore publish several pieces of status information that can safely be examined while the worker is processing data.
Examples include:
number of segments processed
number of samples examined
number of signals detected
whether the worker is currently busy
Your design should use atomic variables where they provide an appropriate solution.
Do not protect every integer in the program with a mutex simply because multiple threads exist.
Part of the assignment is deciding which shared state requires:
atomic operations;
mutex protection;
thread-local storage;
and which state requires no synchronization at all.
System Monitor

In addition to the worker threads, your application must contain a monitoring capability.
Approximately once per second while processing is occurring, the program should display a status report similar to the following:
--------------------------------------------------
Emergency Signal Analysis Status

Worker 0: jobs=14 samples=23145 alerts=2
Worker 1: jobs=11 samples=28442 alerts=1
Worker 2: jobs=17 samples=19821 alerts=3
Worker 3: jobs=13 samples=24531 alerts=1

Total completed: 55
Jobs remaining:   127
--------------------------------------------------
The precise format is your choice.
The monitoring operation must not significantly interfere with workers performing signal analysis.
The monitor should be able to examine worker statistics while the workers continue working.
Thread-Local State

A signal-analysis worker repeatedly performs essentially the same processing operation. Recreating temporary working objects for every new segment may therefore be unnecessary.
Your implementation must make meaningful use of several thread_local data items.
Suitable uses might include:
reusable temporary storage;
analysis buffers;
parsing state;
worker-specific formatting objects;
intermediate statistics;
worker-specific diagnostic information.
These are examples, not required variable names or structures.
Design question:
What information belongs to the worker thread itself rather than to a particular signal segment or to the entire application?
Your program must contain at least three meaningful pieces of thread-local state.
Your documentation must explain why each was made thread-local.
Results

Each signal segment must generate exactly one final result.
Results may appear in any order because different workers will finish at different times.
S021 samples=2350 min=3 max=94 avg=34.7 above=47 longest=12 STRONG SIGNAL
S004 samples=970  min=1 max=28 avg=12.2 above=0  longest=0  NO SIGNAL
S019 samples=5100 min=2 max=73 avg=26.1 above=13 longest=4  POSSIBLE SIGNAL
You may either:
print results as they become available; or
store the results and print them after processing has completed.
Whichever approach you choose must be thread safe.
Output from different threads must not become intermixed or corrupted.
Program Termination

Your program must terminate cleanly.
When all signal segments have been processed:
workers must not remain blocked indefinitely;
the monitoring operation must terminate;
all worker threads must be joined;
synchronization objects must remain valid until no thread needs them;
the program must produce a final summary.
For example:
Analysis Complete

Segments processed:     182
Samples examined:   3,421,892
Signals detected:          27
Worker threads:             4
Elapsed time:           2.84 seconds
Performance Experiment

Run the program on the same sufficiently large input file using:
1 worker
2 workers
4 workers
8 workers
Record the execution time for each run and include the results in the README.
For example:
Workers	Time
1	8.42 s
2	4.61 s
4	2.71 s
8	2.54 s

Briefly discuss the results.
In particular, consider why doubling the number of threads does not necessarily cut the execution time in half.
Design Freedom

This specification intentionally does not define classes or provide a required architecture.
You must decide how to represent:
signal segments;
analysis results;
jobs;
workers;
the thread pool;
the job queue;
worker statistics;
synchronization;
shutdown.
A successful solution could contain classes such as:
SignalSegment
SignalResult
ThreadPool
WorkerStatus
but these classes are neither required nor necessarily the best design.
You are expected to make reasonable software-engineering decisions.
Restrictions and Technical Requirements

Your solution must:
compile using C++11;
use std::thread and a reusable pool of worker threads;
dynamically distribute jobs among workers;
use appropriate synchronization and avoid unnecessary busy waiting;
make meaningful use of std::atomic;
maintain worker information that can safely be observed during execution;
make meaningful use of at least three thread_local data items;
terminate cleanly;
contain no data races.
You may use facilities including:
std::thread
std::mutex
std::condition_variable
std::atomic
thread_local
std::queue
std::vector
std::chrono
Use of these facilities is not intended to prescribe a particular design.
Compilation

Your program must compile on the departmental Linux system using a command similar to:
g++ -std=c++11 -Wall -Wextra -pthread Signal_Hunters.cpp Signal_Hunters_Driver.cpp -o signalHunter
Your submitted program should compile without warnings.
Required Submission

Submit:
Signal_Hunters_Driver.cpp
Signal_Hunters.cpp
Signal_Hunters.h
README.txt
The README must contain:
a brief description of the program architecture;
an explanation of how jobs are distributed;
identification of the shared data in the program;
identification of the atomic variables and why atomic operations are appropriate;
identification of the thread-local variables and why they belong to individual threads;
an explanation of how workers sleep when no work is available;
an explanation of how workers know when to terminate;
the results of the 1-, 2-, 4-, and 8-worker performance experiment.
Grading - 100 Points

Category	Points
Correct signal processing	15
Functional reusable thread pool	20
Dynamic distribution of work	10
Correct synchronization	15
Meaningful use of atomic state	10
Meaningful use of thread_local storage	10
Monitoring and worker statistics	5
Clean startup and termination	5
Performance experiment and discussion	5
Code quality, documentation, and Makefile	5
Evaluation Considerations

A program that merely produces the correct signal-analysis results is not sufficient.
The primary question is whether the application demonstrates an understanding of concurrent program design.
Particular attention will be given to whether:
worker threads actually perform multiple jobs;
workers dynamically obtain work;
workers avoid unnecessary busy waiting;
shared objects are protected appropriately;
atomic variables are used for operations that are actually atomic in nature;
thread-local data represents worker-specific state rather than simply being included to satisfy the requirement;
the application shuts down without deadlock;
output remains valid under repeated executions.
Programs may be tested using considerably larger inputs than the sample input.
Questions to Consider Before Coding

What constitutes a job?
Who creates the jobs?
Where do jobs wait until a worker becomes available?
How does a sleeping worker discover that new work exists?
Which information belongs to the entire application?
Which information belongs to a particular worker?
Which information belongs to a particular signal-analysis job?
Which values must another thread be able to read while they are changing?
How will the final worker know that there will never be another job?
What prevents the program from terminating while workers are still executing?
If your design has clear answers to these questions, you are probably ready to begin implementing the system.
