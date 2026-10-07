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

Input: `signals.txt` (500 segments, 4,165,467 samples). 5 runs per configuration. Output was identical every run (345 signals detected). 
**Machine 1 (local):** MacBook Pro (Mac17,2), Apple M5, 10 cores (4 Super + 6 Efficiency).

| Workers | Analysis time (median reported) | Total program time (avg) |
|---------|---------------------------------|--------------------------|
| 1       | 0.003 s                         | 0.052 s                  |
| 2       | 0.001 s                         | 0.051 s                  |
| 4       | 0.001 s                         | 0.050 s                  |
| 8       | 0.001 s                         | 0.050 s                  |
| 16      | 0.001 s                         | 0.049 s                  |
| 32      | 0.002 s                         | 0.050 s                  |
| 64      | 0.002 s                         | 0.050 s                  |
| 128     | 0.003 s                         | 0.054 s                  |
| 256     | 0.004 s                         | 0.053 s                  |
| 512     | 0.008 s                         | 0.057 s                  |
| 1024    | 0.016 s                         | 0.066 s                  |

Analysis time drops from 1 to 2 workers, then flattens from 2 to 16 workers. It gets worse from 32 workers on, doubling at each step from 128 to 1024. The analysis takes only a few milliseconds, so thread startup and queue-lock contention are a large share of it. With only 10 CPUs, extra threads beyond that cannot run in parallel. They only add creation, scheduling and join cost, which grows with the thread count. File parsing (~50 ms) runs on the main thread, so total program time barely changes until the thread overhead becomes large at 512 and 1024.

### Linux server results (earth.ecs.baylor.edu)

**Machine 2:** Baylor ECS shared Linux server `earth`, compiled with `g++ -std=c++17 -Wall -pthread`. Same input (`signals.txt`), one run per configuration. Output was identical for every run: 500 segments, 4,165,467 samples, 345 signals detected. CPU: Intel Xeon Gold 6230 @ 2.10 GHz, 80 logical CPUs (shared with other users).

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

On Linux the analysis scales with workers from 1 to 16 (0.024 s to 0.004 s, about 6x), then gets slightly worse at 32 and 64 as thread overhead outweighs the gains. The Mac is much faster at 1 worker (0.003 s vs 0.024 s), so it has little room left to gain from parallelism. Linux has more room, so the speedup is visible.

At 128 workers the program aborted because `earth` refused to create more threads. This is the per-user process/thread limit on the shared server: `ulimit -u` reports 128, and on Linux each thread counts toward that limit. Counting down from 128, 116 workers was the most that ran, which leaves about 12 slots for the main thread, the monitor thread, the shell and other processes. The 128 workers plus the main thread, the monitor thread, and the login shell and other processes already running exceed it. The program does not catch the exception from `std::thread`. macOS handled up to 1024 workers.
