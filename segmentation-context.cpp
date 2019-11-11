
// Windows
#include <tchar.h>
#include <windows.h>
#include <windows.data.json.h>

// OpenCV
#include <opencv2/core.hpp>

// std
#include <stdio.h>
#include <string>
#include <iostream>
#include <filesystem>

// using
using namespace std;

// to be implemented
void generate_json();
void generate_bat();

// work in progress
// trying to get recursive file reader up
void convert_segments(const string path)
{
	for (const auto& dirEntry : filesystem::recursive_directory_iterator(path))
		if (!dirEntry.is_directory()) cout << dirEntry.path() << endl;
	{	// now add code that converts files found to base64 strings
		// then generate json
		// then generate bats
	}
}

int main(int argc, char** argv)
{	
	convert_segments("segments");
	return 0;
}
