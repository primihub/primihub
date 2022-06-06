#ifndef HEADER_UTILS_H_
#define HEADER_UTILS_H_

#include <math.h>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

#define LOG_DEBUG_UTILS 0




double calculateSD(vector<double> data);
int printingFunction();



/**************************************************************************/
double calculateSD(vector<double> data)
{
	double sum = 0, mean, standardDeviation = 0;
    for(int i = 0; i < data.size(); ++i)
        sum += data[i];
    mean = sum/data.size();

    for(int i = 0; i < data.size(); ++i)
        standardDeviation += (data[i] - mean)*(data[i] - mean);

    return sqrt(standardDeviation / 10);
}


int printingFunction()
{

#if LOG_DEBUG_UTILS
	cout << "Hello" << endl;
#endif
	return 0;
}



#endif /* HEADER_UTILS_H_ */
