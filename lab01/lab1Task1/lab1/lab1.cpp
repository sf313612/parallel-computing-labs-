#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>

using namespace std;

// calculate vector norm
double normCalculation(const vector<double>& vec) 
{
    double sum = 0.0;
    for (int i = 0; i < vec.size(); i++) {
        sum += (vec[i] * vec[i]);
    }
    return sqrt(sum);
}

int main()
{
    const int n = 50000000;
    vector<double> vec(n);

    //fill vector with random numbers
    srand(time(0));
    generate(vec.begin(), vec.end(), []() {
        return rand() % 100;
    });
     
    volatile double result = 0.0;

    auto start = chrono::high_resolution_clock::now();

    result = normCalculation(vec);

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration = end - start;

    cout << "Vector norm: " << result << endl;
    cout << "Execution time (ms): " << duration.count() << endl;

    return 0;
}