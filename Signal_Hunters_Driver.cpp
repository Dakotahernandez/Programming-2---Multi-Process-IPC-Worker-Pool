#include <iostream>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <queue>
#include <sstream>

#include "Signal_Hunters.h"

using namespace std;



int main(int argc, char *argv[]) {//example call ./signalHunter signals.txt 4


    string filename = argv[1];
    string line;

    istringstream iss(line);
    string signal_id;
    int value;

    int number_of_threads = stoi(argv[2]);
    ifstream fin(filename);

    vector<int> signals;
    queue<Segment> job_queue;
    vector<thread> workers;


    if (fin.is_open()) {
        /*
        example line S001 70 12 15 17 74 82 91 78 20 18 14 11
        */
        while (getline(fin, line)) {
            
            iss >> signal_id;
            while (iss >> value) {
                signals.push_back(value);
            }
            Segment segment;
            segment.signals = signals;
            segment.segment_id = signal_id;
            segment.numner_of_signals = signals.size();
            job_queue.push(segment);
            signals.clear();
        }
    }
    else {
        cout << "Error opening file: " << filename << endl;
    }



    return 0;
}

