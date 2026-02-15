#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <thread>

using namespace std;

// calculate partial sum of squares for vector
void partialNormCalculation(const vector<double>& vec, int start, int end, double& result)
{
    double sum = 0.0;
    for (int i = start; i < end; i++) {
        sum += (vec[i] * vec[i]);
    }
    result = sum;
}

int main()
{
    const int n = 50000000;

    // number of logical CPU cores
    unsigned int numThreads = thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 2;

    vector<double> vec(n);
    vector<double> partialResults(numThreads);

    // fill vector with random numbers
    srand(time(0));
    generate(vec.begin(), vec.end(), []() {
        return rand() % 100;
    });

    vector<thread> threads;

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

    double result = sqrt(totalSum);

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration = end - start;

    cout << "Vector norm: " << result << endl;
    cout << "Parallel execution time (ms): " << duration.count() << endl;

    return 0;
}