#ifndef Signal_Hunters_H
#define Signal_Hunters_H

#include <iostream>

using namespace std;


/*
upload site expected output 
.
.
C09983 samples=49 min=28 max=87 avg=49.16 above=9 longest=9 STRONG SIGNAL worker=0
C09984 samples=66 min=29 max=64 avg=43.95 above=0 longest=0 NO SIGNAL worker=0
C09985 samples=83 min=30 max=76 avg=48.33 above=7 longest=1 POSSIBLE SIGNAL worker=0
C09986 samples=100 min=31 max=90 avg=50.64 above=7 longest=7 STRONG SIGNAL worker=0
C09987 samples=56 min=33 max=66 avg=47.79 above=0 longest=0 NO SIGNAL worker=0
C09988 samples=73 min=33 max=79 avg=52.73 above=6 longest=1 POSSIBLE SIGNAL worker=0
C09989 samples=90 min=34 max=93 avg=53.61 above=10 longest=10 STRONG SIGNAL worker=0
C09990 samples=46 min=35 max=70 avg=52.78 above=0 longest=0 NO SIGNAL worker=0
C09991 samples=63 min=37 max=82 avg=53.52 above=5 longest=1 POSSIBLE SIGNAL worker=0
C09992 samples=80 min=37 max=96 avg=56.84 above=8 longest=8 STRONG SIGNAL worker=0
C09993 samples=97 min=38 max=73 avg=54.86 above=0 longest=0 NO SIGNAL worker=0
C09994 samples=53 min=39 max=83 avg=57.77 above=4 longest=1 POSSIBLE SIGNAL worker=0
C09995 samples=70 min=40 max=99 avg=61.73 above=6 longest=6 STRONG SIGNAL worker=0
C09996 samples=87 min=20 max=55 avg=37.53 above=0 longest=0 NO SIGNAL worker=0
C09997 samples=43 min=21 max=65 avg=39.65 above=4 longest=1 POSSIBLE SIGNAL worker=0
C09998 samples=60 min=22 max=81 avg=46.37 above=9 longest=9 STRONG SIGNAL worker=0
C09999 samples=77 min=23 max=57 avg=39.83 above=0 longest=0 NO SIGNAL worker=0

Analysis Complete
=================
Segments processed: 10000
Samples examined:   699985
Signals detected:   6666
Worker threads:     1
Elapsed time:       0.032 seconds


*/

struct Segment{
    vector<int> signals;
    string segment_id = NULL;
    int numner_of_signals = 0;
    int min_signal_strength = 0;
    int max_signal_strength = 0;
    double average_signal_strength;
    int number_of_signal_above_detection_threshold = 0;
    int longest_consecutive_run_above_detection_threshold = 0;

    /*
    Each segment must also be classified. A reasonable
     classification scheme might use:
    NO SIGNAL.      0
    POSSIBLE SIGNAL 1
    STRONG SIGNAL   3
    */
    int classification = 0;
    
    /*
      Emergency-response supervisors want to observe the system 
      while it is running. Each worker must therefore publish 
      several pieces of status information that can safely be 
      examined while the worker is processing data.
     */
    int number_of_segments_processed = 0;
    int number_of_samples_examined = 0;
    int number_of_signals_detected = 0;
    bool is_worker_busy = false;
};

/*
Approximately once per second while processing is occurring, 
the program should display a status report similar to the following:
ex from description 
--------------------------------------------------
Emergency Signal Analysis Status

Worker 0: jobs=14 samples=23145 alerts=2
Worker 1: jobs=11 samples=28442 alerts=1
Worker 2: jobs=17 samples=19821 alerts=3
Worker 3: jobs=13 samples=24531 alerts=1

Total completed: 55
Jobs remaining:   127
--------------------------------------------------


*/
void print_monitoring_status(Segment segment, ostream& pout);


/*
Results may appear in any order because different workers will finish at different times.
S021 samples=2350 min=3 max=94 avg=34.7 above=47 longest=12 STRONG SIGNAL
S004 samples=970  min=1 max=28 avg=12.2 above=0  longest=0  NO SIGNAL
S019 samples=5100 min=2 max=73 avg=26.1 above=13 longest=4  POSSIBLE SIGNAL
You may either:
print results as they become available; or
store the results and print them after processing has completed.
Whichever approach you choose must be thread safe.
Output from different threads must not become intermixed or corrupted.
*/
void print_segment_results(Segment segment, ostream& pout);



/*
program termination 

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
*/



#endif 