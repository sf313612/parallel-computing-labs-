#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>

using namespace std;

double normCalculation(const vector<double>& vec)
{
    double sum = 0.0;
    for (int i = 0; i < vec.size(); i++) {
        sum += vec[i] * vec[i];
    }
    return sqrt(sum);
}

int main()
{
    vector<double> vec = { 1, 2, 3, 4 };

    cout << "Vector elements: ";
    for (double v : vec)
        cout << v << " ";
    cout << endl;

    double result = normCalculation(vec);

    cout << "Vector norm: " << result << endl;

    return 0;
}
