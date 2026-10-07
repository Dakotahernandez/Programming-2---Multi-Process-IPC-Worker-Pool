#include "Signal_Hunters.h"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>

using namespace std;


thread_local int tl_worker_id = -1;
thread_local WorkerStatus* tl_worker_status = nullptr;


const char* classification_name(int classification) {
    switch (classification) {
        case 2:
            return "STRONG SIGNAL";
        case 1:
            return "POSSIBLE SIGNAL";
        default:
            return "NO SIGNAL";
    }
}


void parse_segment(const string& line, Segment& segment) {
    // line format: <id> <threshold> <sample> <sample> ...
    size_t space = line.find(' ');
    segment.segment_id = line.substr(0, space);

    const char* p = line.c_str() + space;
    char* end = nullptr;
    segment.detection_threshold = strtol(p, &end, 10);
    p = end;

    while (true) {
        int value = strtol(p, &end, 10);
        if (end == p) {
            break;
        }
        segment.signals.push_back(value);
        p = end;
    }

    segment.numner_of_signals = segment.signals.size();
}


void analyze_segment(Segment& segment) {
    const vector<int>& samples = segment.signals;

    int sum = 0;
    int current_run = 0;
    int min_value = samples[0];
    int max_value = samples[0];
    for (size_t i = 0; i < samples.size(); ++i) {
        int value = samples[i];
        sum += value;
        if (value < min_value) {
            min_value = value;
        }
        if (value > max_value) {
            max_value = value;
        }

        if (value > segment.detection_threshold) {
            ++segment.number_of_signal_above_detection_threshold;
            ++current_run;
            if (current_run > segment.longest_consecutive_run_above_detection_threshold) {
                segment.longest_consecutive_run_above_detection_threshold = current_run;
            }
        }
        else {
            current_run = 0;
        }
    }

    segment.min_signal_strength = min_value;
    segment.max_signal_strength = max_value;
    segment.average_signal_strength = (double)sum / samples.size();

    if (segment.longest_consecutive_run_above_detection_threshold >= 3) {
        segment.classification = 2;
    }
    else if (segment.number_of_signal_above_detection_threshold > 0) {
        segment.classification = 1;
    }
    else {
        segment.classification = 0;
    }
}


void print_segment_results(Segment segment, ostream& pout) {
    pout << segment.segment_id
         << " samples=" << segment.numner_of_signals
         << " min=" << segment.min_signal_strength
         << " max=" << segment.max_signal_strength
         << " avg=" << fixed << setprecision(2) << segment.average_signal_strength
         << " above=" << segment.number_of_signal_above_detection_threshold
         << " longest=" << segment.longest_consecutive_run_above_detection_threshold
         << ' ' << classification_name(segment.classification)
         << " worker=" << tl_worker_id;
}


string format_segment_result(const Segment& segment) {
    ostringstream format;
    print_segment_results(segment, format);
    return format.str();
}


ThreadPool::ThreadPool(int number_of_workers)
    : statuses(number_of_workers), jobs_pending(0) {
    for (int i = 0; i < number_of_workers; ++i) {
        workers.push_back(thread(&ThreadPool::worker_loop, this, i));
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::submit(Job job) {
    jobs_pending.fetch_add(1);
    {
        lock_guard<mutex> lock(queue_mutex);
        job_queue.push(std::move(job));
    }
    job_available.notify_one();
}

void ThreadPool::shutdown() {
    {
        lock_guard<mutex> lock(queue_mutex);
        stopping = true;
    }
    job_available.notify_all();
    for (size_t i = 0; i < workers.size(); ++i) {
        if (workers[i].joinable()) {
            workers[i].join();
        }
    }
}

int ThreadPool::size() const {
    return statuses.size();
}

int ThreadPool::jobs_remaining() const {
    return jobs_pending.load();
}

const WorkerStatus& ThreadPool::status(int worker) const {
    return statuses[worker];
}

WorkerStatus* ThreadPool::current_worker_status() {
    return tl_worker_status;
}

void ThreadPool::worker_loop(int worker_id) {
    tl_worker_id = worker_id;
    tl_worker_status = &statuses[worker_id];

    while (true) {
        Job job;
        {
            unique_lock<mutex> lock(queue_mutex);
            while (!stopping && job_queue.empty()) {
                job_available.wait(lock);
            }
            if (job_queue.empty()) {
                return;
            }
            job = std::move(job_queue.front());
            job_queue.pop();
        }

        tl_worker_status->is_busy.store(true);
        job();
        tl_worker_status->is_busy.store(false);
        jobs_pending.fetch_sub(1);
    }
}


Monitor::Monitor(const ThreadPool& pool, ostream& pout) : pool(pool), pout(pout) {
}

Monitor::~Monitor() {
    stop();
}

void Monitor::start() {
    monitor_thread = thread(&Monitor::run, this);
}

void Monitor::stop() {
    {
        lock_guard<mutex> lock(monitor_mutex);
        stop_requested = true;
    }
    stop_signal.notify_one();
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
}

void Monitor::run() {
    unique_lock<mutex> lock(monitor_mutex);
    while (!stop_requested) {
        cv_status status = stop_signal.wait_for(lock, chrono::seconds(1));
        if (status == cv_status::timeout && !stop_requested) {
            lock.unlock();
            print_status();
            lock.lock();
        }
    }
    lock.unlock();
    print_status();
}

void Monitor::print_status() {
    ostringstream report;
    int total_completed = 0;

    report << "--------------------------------------------------\n"
           << "Emergency Signal Analysis Status\n\n";
    for (int i = 0; i < pool.size(); ++i) {
        const WorkerStatus& status = pool.status(i);
        int jobs = status.segments_processed.load();
        total_completed += jobs;
        report << "Worker " << i
               << ": jobs=" << jobs
               << " samples=" << status.samples_examined.load()
               << " alerts=" << status.signals_detected.load()
               << (status.is_busy.load() ? " busy" : " idle")
               << '\n';
    }
    report << "\nTotal completed: " << total_completed << '\n'
           << "Jobs remaining:  " << pool.jobs_remaining() << '\n'
           << "--------------------------------------------------\n";

    pout << report.str() << flush;
}


void print_summary(const ThreadPool& pool, double elapsed_seconds, ostream& pout) {
    int segments = 0;
    int samples = 0;
    int signals = 0;
    for (int i = 0; i < pool.size(); ++i) {
        const WorkerStatus& status = pool.status(i);
        segments += status.segments_processed.load();
        samples += status.samples_examined.load();
        signals += status.signals_detected.load();
    }

    pout << "\nAnalysis Complete\n"
        << "=================\n"
        << "Segments processed: " << segments << '\n'
        << "Samples examined:   " << samples << '\n'
        << "Signals detected:   " << signals << '\n'
        << "Worker threads:     " << pool.size() << '\n'
        << "Elapsed time:       " << fixed << setprecision(3) << elapsed_seconds << " seconds\n";
}
