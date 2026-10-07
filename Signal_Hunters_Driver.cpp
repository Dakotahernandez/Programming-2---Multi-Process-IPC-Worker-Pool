#include <iostream>
#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>

#include "Signal_Hunters.h"

using namespace std;

// One job: analyze a single segment, update this worker's counters, store the result.
struct AnalyzeJob {
    Segment* segment;
    vector<string>* results;
    mutex* results_mutex;

    AnalyzeJob(Segment* segment, vector<string>* results, mutex* results_mutex)
        : segment(segment), results(results), results_mutex(results_mutex) {
    }

    void operator()() {
        analyze_segment(*segment);

        WorkerStatus* status = ThreadPool::current_worker_status();
        status->segments_processed.fetch_add(1);
        status->samples_examined.fetch_add(segment->numner_of_signals);
        if (segment->classification != 0) {
            status->signals_detected.fetch_add(1);
        }

        string result = format_segment_result(*segment);
        lock_guard<mutex> lock(*results_mutex);
        results->push_back(result);
    }
};




int main(int, char *argv[]) {//example call ./signalHunter signals.txt 4

    string filename = argv[1];
    int number_of_threads = atoi(argv[2]);

    ifstream fin(filename);
    vector<string> lines;
    string line;
    /*
    example line S001 70 12 15 17 74 82 91 78 20 18 14 11
    */
    while (getline(fin, line)) {
        lines.push_back(line);
    }
    fin.close();

    // main thread parses the file into segments
    vector<Segment> segments(lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
        parse_segment(lines[i], segments[i]);
    }

    vector<string> results;
    results.reserve(lines.size());
    mutex results_mutex;

    chrono::steady_clock::time_point start = chrono::steady_clock::now();

    ThreadPool pool(number_of_threads);
    Monitor monitor(pool, cout);
    monitor.start();

    for (size_t i = 0; i < segments.size(); ++i) {
        pool.submit(AnalyzeJob(&segments[i], &results, &results_mutex));
    }

    pool.shutdown();
    chrono::steady_clock::time_point finish = chrono::steady_clock::now();
    monitor.stop();

    for (size_t i = 0; i < results.size(); ++i) {
        cout << results[i] << '\n';
    }

    double elapsed = chrono::duration<double>(finish - start).count();
    print_summary(pool, elapsed, cout);

    return 0;
}
