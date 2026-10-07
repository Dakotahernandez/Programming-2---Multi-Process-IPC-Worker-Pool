# Signal Hunters – Submission

![Diagram](IMG_2371.png)

## Compilation

```bash
g++ -std=c++11 -Wall -Wextra -pthread Signal_Hunters.cpp
 Signal_Hunters_Driver.cpp -o signalHunter
```

## Architecture

- **Main thread:** parses the file into `Segment`s, submits one job per segment, calls `shutdown()`, prints results and summary.
- **ThreadPool:** N worker threads, a job queue, a mutex and a condition variable.
- **Monitor:** separate thread that prints worker status about once per second.
- **Job:** an `AnalyzeJob` function object that analyzes one segment, updates the worker's counters, and appends the formatted result to the results vector.

## Job Distribution

Each segment is one job on a single shared queue. Idle workers take the next job from the front, so work is ditributed dynamically.

## Shared Data

| Data | Protection |
|---|---|
| `job_queue`, `stopping` | `queue_mutex` + `job_available` |
| `results` vector | `results_mutex` |
| `Monitor::stop_requested` | `monitor_mutex` + `stop_signal` |
| `segments` vector | None needed. Built before workers start, and each job touches only its own element. |
| `statuses` vector | Each worker writes only its own entry. Monitor reads via atomics. |

## Atomic Variables

| Variable | Why atomic |
|---|---|
| `segments_processed`, `samples_examined`, `signals_detected` | Written by one worker, read by the monitor and summary while running. Independent counters need no mutex. |
| `is_busy` | Flag set by the worker, read by the monitor. |
| `ThreadPool::jobs_pending` | `fetch_add` in `submit`, `fetch_sub` when a job finishes, read by the monitor as "jobs remaining". |

## Thread-Local Variables

| Variable | Why thread-local |
|---|---|
| `tl_worker_id` | The worker's own index, used to tag each result with `worker=N`. |
| `tl_worker_status` | Pointer to this worker's `WorkerStatus`, so a job can update its worker's stats without a lookup or lock. |

## Sleeping When No Work Is Available

Workers block on `job_available.wait()` until the queue is non-empty or `stopping` is set. No CPU is used while waiting. `submit()` pushes under the mutex and calls `notify_one()`.

## Termination

`shutdown()` sets `stopping = true` under the mutex and calls `notify_all()`. Workers drain any remaining jobs, and each exits when the queue is empty and `stopping` is set. `shutdown()` joins all workers. The monitor is stopped through its own condition variable and joined.

## Classification

- **STRONG SIGNAL:** longest consecutive run above the threshold is 3 or more.
- **POSSIBLE SIGNAL:** at least one sample above the threshold, but not strong.
- **NO SIGNAL:** no samples above the threshold.

"Above" means strictly greater than the threshold. "Signals detected" counts STRONG and POSSIBLE segments.

## Performance Experiment

Input: `signals.txt` (500 segments, 4,165,467 samples). Output was identical for every run on both machines: 500 segments, 4,165,467 samples, 345 signals detected. The reported time is the "Elapsed time" printed by the program, which covers creating the pool through `shutdown()` and excludes file parsing.

### Local results (MacBook Pro)

**Machine 1:** MacBook Pro (Mac17,2), Apple M5, 10 cores (4 Super + 6 Efficiency), 16 GB RAM, macOS (Darwin 25.6.0). Compiled with the Makefile: `g++ -std=c++11 -Wall -Wextra -pthread -O2`. 5 runs per configuration, median reported.

| Workers | Elapsed time |
|---------|--------------|
| 1       | 0.003 s      |
| 2       | 0.001 s      |
| 4       | 0.001 s      |
| 8       | 0.001 s      |
| 16      | 0.001 s      |
| 32      | 0.002 s      |
| 64      | 0.002 s      |
| 128     | 0.003 s      |
| 256     | 0.004 s      |
| 512     | 0.008 s      |
| 1024    | 0.016 s      |

Time drops from 1 to 2 workers, then flattens through 16. It gets worse from 32 workers on, doubling at each step from 128 to 1024. The analysis is only a few milliseconds, so thread startup and queue-lock contention are a large share of it. With 10 cores, extra threads cannot run in parallel and only add creation, scheduling and join cost. The whole-process time, which includes the ~47 ms file parse on the main thread, stayed at about 0.05 s for 1 to 64 workers and rose to 0.066 s at 1024.

### Linux server results (earth.ecs.baylor.edu)

**Machine 2:** Baylor ECS shared Linux server `earth`, Intel Xeon Gold 6230 @ 2.10 GHz, 80 logical CPUs (shared with other users). Compiled with `g++ -std=c++17 -Wall -pthread` (no `-O`). One run per configuration.

| Workers | Elapsed time |
|---------|--------------|
| 1       | 0.024 s      |
| 2       | 0.014 s      |
| 4       | 0.007 s      |
| 8       | 0.005 s      |
| 16      | 0.004 s      |
| 32      | 0.006 s      |
| 64      | 0.006 s      |
| 116     | 0.009 s (highest count that ran) |
| 128     | crashed (`std::system_error: Resource temporarily unavailable`) |

On Linux the time scales with workers from 1 to 16 (0.024 s to 0.004 s, about 6x), then gets slightly worse at 32 and 64 as thread overhead outweighs the gains. The Mac is faster at every worker count and much faster at 1 worker (0.003 s vs 0.024 s), so it has little room left to gain from parallelism, while earth's slower baseline leaves more for threads to recover. Part of the gap may be the missing `-O` flag on earth, which has not been re-tested.

At 128 workers the program aborted because `earth` refused to create more threads. `ulimit -u` reports 128 on earth, and each thread counts toward that per-user limit. Counting down from 128, 116 workers was the most that ran, which leaves about 12 slots for the main thread, the monitor thread, the shell and other processes. The program does not catch the exception from `std::thread`. macOS handled up to 1024 workers.
