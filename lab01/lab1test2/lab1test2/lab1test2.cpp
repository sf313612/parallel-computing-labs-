#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <thread>

using namespace std;

// size for functions
vector<int> sizes = { 500000, 1000000, 1500000, 20000000, 50000000 };

// calculate partial sum of squares for vector
void partialNormCalculation(const vector<double>& vec, int start, int end, double& result)
{
    double sum = 0.0;
    for (int i = start; i < end; i++) {
        sum += (vec[i] * vec[i]);
    }
    result = sum;
}

double normCalculation(const vector<double>& vec)
{
    double sum = 0.0;
    for (int i = 0; i < vec.size(); i++) {
        sum += vec[i] * vec[i];
    }
    return sqrt(sum);
}

void calculateTimeForPar(const vector<double>& vec, int numThreads)
{
    int runs = 5;
    double totalTime = 0.0;

    for (int r = 0; r < runs; r++) {
        vector<double> partialResults(numThreads);
        vector<thread> threads;
        int n = vec.size();

        // size of vector part for threads
        int blockSize = n / numThreads;

        auto start = chrono::high_resolution_clock::now();

        for (unsigned int i = 0; i < numThreads; i++) {
            int blockStart = i * blockSize;
            int blockEnd = blockStart + blockSize;

            // for the last thread
            if (i == numThreads - 1)
                blockEnd = n;

            thread t(partialNormCalculation, cref(vec), blockStart, blockEnd, ref(partialResults[i]));
            threads.push_back(move(t));
        }

        // till every thread finished
        for (auto& t : threads)
            t.join();

        double totalSum = 0.0;
        for (double s : partialResults)
            totalSum += s;

        volatile double result = sqrt(totalSum);

        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double, milli> duration = end - start;
        totalTime += duration.count();
    }

    cout << " Time (ms, average of " << runs << " runs): " << totalTime / runs << endl;
}

void calculateTime(const vector<double>& vec) 
{
    int runs = 5;
    double totalTime = 0.0;
    for (int i = 0; i < runs; i++)
    {
        auto start = chrono::high_resolution_clock::now();

        volatile double result = normCalculation(vec);

        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double, milli> duration = end - start;
        totalTime += duration.count();
    }

    cout << "Time (ms, average of " << runs << " runs): " << totalTime / runs << endl;
}

vector<double> createVector(int n) {
    vector<double> vec(n);

    // fill vector with random numbers
    generate(vec.begin(), vec.end(), []() {
        return rand() % 100;
        });

    return vec;
}

void testFunction() 
{
    // number of logical CPU cores
    unsigned int maxThreads = thread::hardware_concurrency();
    if (maxThreads == 0)
        maxThreads = 8;

    for (int n : sizes) {

        cout << "Vector size: " << n << endl;

        vector<double> vec = createVector(n);

        cout << "Sequential calculations:" << endl;
        calculateTime(vec);

        cout << "Parallel calculations: " << endl;
        for (int threads : {1, 2, 4, 6, 12}) {
            cout << "Threads: " << threads;
            calculateTimeForPar(vec, threads);
        }
  
        cout << endl;
    }
}

int main()
{
    srand(time(0));

    testFunction();

    return 0;
}
