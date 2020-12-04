#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <cstdlib>
#include <sstream>
#include <list>
#include <string>

using namespace std;

// a class to get more accurate timeB

class stopwatch
{

private:
	double elapsedTime;
	double startedTime;
	bool timing;
	//returns current time in seconds
	double current_time()
	{
		timeval tv;
		gettimeofday(&tv, NULL);
		double rtn_value = (double)tv.tv_usec;
		rtn_value /= 1e6;
		rtn_value += (double)tv.tv_sec;
		return rtn_value;
	}

public:
	stopwatch() : elapsedTime(0), startedTime(0), timing(false)
	{
	}

	void start()
	{
		if (!timing)
		{
			timing = true;
			startedTime = current_time();
		}
	}

	void stop()
	{
		if (timing)
		{
			elapsedTime += current_time() - startedTime;
			timing = false;
		}
	}

	void resume()
	{
		start();
	}

	void reset()
	{
		elapsedTime = 0;
		startedTime = 0;
		timing = false;
	}

	double getTime()
	{
		return elapsedTime;
	}
};

// function takes an array pointer, and the number of rows and cols in the array, and
// allocates and intializes the two dimensional array to a bunch of random numbers

void makeRandArray(unsigned int **&data, unsigned int rows, unsigned int cols, unsigned int seed)
{
	// allocate the array
	data = new unsigned int *[rows];
	for (unsigned int i = 0; i < rows; i++)
	{
		data[i] = new unsigned int[cols];
	}

	// seed the number generator
	// you should change the seed to get different values
	srand(seed);

	// populate the array

	for (unsigned int i = 0; i < rows; i++)
		for (unsigned int j = 0; j < cols; j++)
		{
			data[i][j] = rand() % 10000 + 1; // number between 1 and 10000
		}
}

void getDataFromFile(unsigned int **&data, char fileName[], unsigned int &rows, unsigned int &cols)
{
	ifstream in;
	in.open(fileName);
	if (!in)
	{
		cerr << "error opening file: " << fileName << endl;
		exit(-1);
	}

	in >> rows >> cols;
	data = new unsigned int *[rows];
	for (unsigned int i = 0; i < rows; i++)
	{
		data[i] = new unsigned int[cols];
	}

	// now read in the data

	for (unsigned int i = 0; i < rows; i++)
		for (unsigned int j = 0; j < cols; j++)
		{
			in >> data[i][j];
		}
}

int main(int argc, char *argv[])
{

	if (argc < 3)
	{
		cerr << "Usage: " << argv[0] << " [input data file] [num of threads to use] " << endl;

		cerr << "or" << endl
			 << "Usage: " << argv[0] << " rand [num of threads to use] [num rows] [num cols] [seed value]" << endl;
		exit(0);
	}

	// read in the file
	unsigned int rows, cols, seed;
	unsigned int numThreads;
	unsigned int **data;
	// convert numThreads to int
	{
		stringstream ss1;
		ss1 << argv[2];
		ss1 >> numThreads;
	}

	string fName(argv[1]);
	if (fName == "rand")
	{
		{
			stringstream ss1;
			ss1 << argv[3];
			ss1 >> rows;
		}
		{
			stringstream ss1;
			ss1 << argv[4];
			ss1 >> cols;
		}
		{
			stringstream ss1;
			ss1 << argv[5];
			ss1 >> seed;
		}
		makeRandArray(data, rows, cols, seed);
	}
	else
	{
		getDataFromFile(data, argv[1], rows, cols);
	}

	// 	cerr << "data: " << endl;
	//  for( unsigned int i = 0; i < rows; i++ ){
	// 	for( unsigned int j = 0; j < cols; j++ ){
	//  		cerr << "i,j,data " << i << ", " << j << ", ";
	//  		cerr << data[i][j] << " ";
	//  	}
	//  	cerr << endl;
	// }
	//  cerr<< endl;

	omp_set_num_threads(numThreads);

	stopwatch S1;
	S1.start();

	/*
	data[i-1][j-1] data[i-1][j] data[i-1][j+1]
	data[i][j-1]                data[i][j+1] 
	data[i+1][j-1] data[i+1][j] data[i+1][j+1]
	*/
	float max = 0;
	int location[2];
	float accum = 0;
	float div = 0;
	float avg = 0;


#pragma omp parallel for private(avg,div,accum)
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			accum = 0;
			div = 1;

			accum += data[i][j];

			if (i - 1 >= 0 && j - 1 >= 0)
			{ // Upper Left
				accum += data[i - 1][j - 1];
				div += 1;
			}
			if (i - 1 >= 0 && j >= 0)
			{ // Upper middle
				accum += data[i - 1][j];
				div += 1;
			}
			if (i - 1 >= 0 && j + 1 < cols)
			{ // Upper Right
				accum += data[i - 1][j + 1];
				div += 1;
			}
			if (i >= 0 && j - 1 >= 0)
			{ // Middle Left
				accum += data[i][j - 1];
				div += 1;
			}
			if (i >= 0 && j + 1 < cols)
			{ // Middle Right
				accum += data[i][j + 1];
				div += 1;
			}
			if (i + 1 < rows && j - 1 >= 0)
			{ // Lower Left
				accum += data[i + 1][j - 1];
				div += 1;
			}
			if (i + 1 < rows && j >= 0)
			{ // Lower Middle
				accum += data[i + 1][j];
				div += 1;
			}
			if (i + 1 < rows && j + 1 < cols)
			{ // Lower Right
				accum += data[i + 1][j + 1];
				div += 1;
			}
			avg = accum / div;
			// cout << avg << "" << endl;
			if (avg > max)
			{
				max = avg;
				location[1] = i;
				location[2] = j;
			}
		}
	}

	cout << "largest average: " << max << endl;
	cout << "found at cells: (" << location[1] << "," << location[2] << ")" << endl;
	S1.stop();

	// print out the max value here

	cerr << "elapsed time: " << S1.getTime() << endl;
}
