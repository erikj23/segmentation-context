
// opencv
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// std
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

// custom
#include "base64.h"

// using
using namespace cv;
using namespace std;

// constants
const string EXTENSION = ".jpg";

const filesystem::path INPUT_PATH("segments");
const filesystem::path OUTPUT_PATH("output");
const filesystem::path SEGMENT_PATH("segments");

// to be implemented
void generate_json();
void generate_bat();

// work in progress
// trying to get recursive file reader up
void convert_segments(vector<string>& encodings, const filesystem::path path)
{
	Mat image;
	vector<uchar> buffer;
	for (const auto& entry : filesystem::recursive_directory_iterator(path))
		if (entry.is_regular_file())
		{
			image = imread(entry.path().string());
			imencode(EXTENSION, image, buffer);
			auto *data = reinterpret_cast<unsigned char*>(buffer.data());
			encodings.push_back(base64_encode(data, buffer.size()));
			// then generate json			
			// then generate bats
		}
}

int main(int argc, char** argv)
{	
	// contains base64 encoded strings of images in the segments folder
	vector<string> encodings;
	convert_segments(encodings, SEGMENT_PATH);
	return 0;
}
