
// 11 2019, Authors:
//	Jeremy Deng
//	Erik Maldonado

// opencv
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// std
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// custom
#include "base64.h"

// namespaces
using namespace cv;
using namespace std;
using namespace std::filesystem;

// constants
const string BAT_FILE("api.bat");
const string API("https://vision.googleapis.com/v1/images:annotate?key=");
const path INPUT_PATH("segments");
const path OUTPUT_PATH("output");
const path SEGMENT_PATH("segments");

// work in progress
void load_key(const string& file_name, string& api_key)
{
	ifstream key_file(file_name);
	if (!getline(key_file, api_key)) printf("load key failure\n");
}

// work in progress
// assuptions:
// outcome:
//	
// improvements:
//	create custom executable class
void generate_bat(const vector<string>& json_strings, const string& file_name, const string& api, const string& api_key)
{
	ofstream bat(file_name);

	for (const string& json: json_strings)
	{
		bat << "curl -v --data " << json << " -S -H \"Content-Type: application/json\" " 
			<< api << api_key << endl;
	}
}

// assumptions:
//	encodings: vector containing proper base64 encoded strings
//	json: properly initialized vector
// outcome:
//	forms strings using json syntax that a vison api request
//		contains base64 encoded image content
//		results are stored in json
// improvements:
//	create a dedicated json class
//	trade constants for variables
//	failure handling
void generate_json(const vector<string>& encodings, vector<string>& json)
{	
	const size_t MAX_RESULTS = 50;
	const string TYPE = "LABEL_DETECTION";
	const string MODEL = "builtin/latest";

	stringstream stream;

	for (auto data : encodings)
	{
		// all c++ json implementations are a headache, therefore:
		stream << "{ \"requests\": [ { \"features\": [ { \"maxResults\": "
			<< MAX_RESULTS << ", \"type\": \"" << TYPE << "\", \"model\": \"" 
			<< MODEL << "\" } ], \"image\": { \"content\": \"" << data << "\" } } ] }";

		json.push_back(stream.str());
		
		// discard contents
		stream.str("");
	}
}

// assumptions:
//	encodings: properly initialized vector
//	path: valid path relative to execution
// outcome:
//	uses base64 protocol to encode jpeg images found in path
//		and stores result in encodings
// improvements:
//	trade constant for variable
//	failure handling
void convert_segments(vector<string>& encodings, const path& path)
{
	const string EXTENSION = ".jpg";

	Mat image;
	vector<uchar> buffer;

	// convert each image found in the path to base64
	for (const auto& entry : filesystem::recursive_directory_iterator(path))
		if (entry.is_regular_file())
		{
			image = imread(entry.path().string());
			buffer.resize(static_cast<size_t>(image.rows) * static_cast<size_t>(image.cols));

			if (imencode(EXTENSION, image, buffer))
				encodings.push_back(base64_encode(buffer.data(), buffer.size()));
			else 
			{ 
				//printf("conversion error on %s\n", entry.path().string());
				printf("conversion failure\n");
			}
		}
}

int main(int argc, char** argv)
{	
	string api_key;
	vector<string> encodings, json_strings, bats;

	// validate(argv)

	load_key(argv[1], api_key);
	convert_segments(encodings, SEGMENT_PATH);
	generate_json(encodings, json_strings);
	generate_bat(json_strings, BAT_FILE, API, api_key);

	return EXIT_SUCCESS;
}
