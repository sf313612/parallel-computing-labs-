#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <random>
#include <climits>
#include <algorithm>

using namespace std;

// create array
vector<int> generateArray(int size) {
    vector<int> arr(size);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(-1000, 1000);

    // fill array with random numbers
    generate(arr.begin(), arr.end(), [&]() {
        return dist(gen);
        });

    return arr;
}

// sequential algorithm
pair<int, int> sequentialMax(const vector<int>& arr) {
    int max = arr[0];
    int count = 1;

    // going throught array
    for (int i = 1; i < arr.size(); i++) {

        // compare next value 
        if (arr[i] > max) {
            max = arr[i];
            count = 1;
        }
        else if (arr[i] == max) { // if max value repeats
            count++;
        }
    }

    return { max, count };
}

// parallel algorithm with mutex
pair<int, int> mutexMax(const vector<int>& arr, int threadCount) {
    int globalMax = INT_MIN;
    int globalCount = 0;

    // create mutex
    mutex mtx;

    // divade array into parts
    int n = arr.size();
    int chunk = n / threadCount;


    vector<thread> threads;
    auto worker = [&](int start, int end) {

        for (int i = start; i < end; i++) {
            // one thread can change max
            lock_guard<mutex> lock(mtx);

            if (arr[i] > globalMax) {
                globalMax = arr[i];
                globalCount = 1;
            }
            else if (arr[i] == globalMax) {
                globalCount++;
            }
        }
    };

    for (int i = 0; i < threadCount; i++) {

        int start = i * chunk;
        int end = start + chunk;

        if (i == threadCount - 1) {
            end = n;
        }

        threads.emplace_back(worker, start, end);
    }

    for (auto& t : threads) 
        t.join();
    
    return { globalMax, globalCount };
}

// parallel algorithm with atomics (CAS)
pair<int, int> atomicMax(const vector<int>& arr, int threadCount) {
    atomic <int> globalMax(INT_MIN);
    atomic <int> globalCount(0);

    // divade array into parts
    int n = arr.size();
    int chunk = n / threadCount;

    vector<thread> threads;
    auto worker = [&](int start, int end) {

        int localCount = 0;
        for (int i = start; i < end; i++) {
            int value = arr[i];
            int currentMax = globalMax.load();

            if (value > currentMax) {
                localCount = 1;

                while (!globalMax.compare_exchange_weak(currentMax, value)) {
                    if (value <= currentMax)
                        break;
                }

                if (globalMax.load() == value) {
                    globalCount.store(localCount); 
                }
            }
            else if (value == currentMax) {
                globalCount.fetch_add(1); 
            }
        }
    };

    for (int i = 0; i < threadCount; i++) {

        int start = i * chunk;
        int end = start + chunk;

        if (i == threadCount - 1) {
            end = n;
        }

        threads.emplace_back(worker, start, end);
    }

    for (auto& t : threads)
        t.join();

    return { globalMax.load(), globalCount.load() };
}

int main()
{
    vector<int> sizes = { 10000, 100000, 1000000, 5000000 };

    int threadsCount = thread::hardware_concurrency();
    cout << "Using threads: " << threadsCount << endl;

    for (int size : sizes) {

        cout << "\nArray size: " << size << endl;

        vector<int> arr = generateArray(size);
        
        auto start = chrono::high_resolution_clock::now();
        auto res1 = sequentialMax(arr);
        auto end = chrono::high_resolution_clock::now();
        
        cout << "Sequential time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms\n";
        
        start = chrono::high_resolution_clock::now();
        auto res2 = mutexMax(arr, threadsCount);
        end = chrono::high_resolution_clock::now();

        cout << "Mutex time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms\n";

        start = chrono::high_resolution_clock::now();
        auto res3 = atomicMax(arr, threadsCount);
        end = chrono::high_resolution_clock::now();
        
        cout << "Atomic time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms\n";
        
        cout << "Max sequential = " << res1.first << " Count = " << res1.second << endl;
        cout << "Max mutex = " << res2.first << " Count = " << res2.second << endl;
        cout << "Max atomic = " << res2.first << " Count = " << res3.second << endl;
    }
}
