
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

// vcpkg
#include <cpprest/json.h>
#include <cpprest/uri.h>

// custom
#include "base64.h"

// namespaces
using namespace cv;
using namespace std;
using namespace std::filesystem;
using namespace web;
using namespace web::json;

// constants
const string API("https://vision.googleapis.com/v1/images:annotate?key=");
const string BAT_FILE("api.bat");
const path INPUT_PATH("segments");
const path OUTPUT_PATH("output");
const path SEGMENT_PATH("segments");

// work in progress
void load_key(const string& file_name, string& api_key)
{
	ifstream key_file(file_name);
	if (!getline(key_file, api_key)) printf("load key failure\n");
}

//todo change to use cpprest
// work in progress
// assuptions:
// outcome:
//	
// improvements:
//	create executable class to retrieve output
void make_request(const vector<string>& json_files, const string& file_name, const string& api, const string& api_key)
{
	ofstream bat(file_name);
	
	for (const string& json_file: json_files)
	{
		bat << "curl --show-error --header \"Content-Type: application/json\" "
			<< "--request POST " << api << api_key << " --data @" << json_file << endl;
	}
	bat.close();
}

//todo change this to use cpprest
// work in progress
// assumptions:
//	encodings: vector containing proper base64 encoded strings
//	json_files: properly initialized vector
// outcome:
//	forms strings using json syntax
//		strings written to files
//		file_names are stored in json_files
// improvements:
//	trade constants for variables
//	failure handling
void generate_json(const vector<string>& encodings, vector<string>& json_files)
{	
	const size_t MAX_RESULTS = 50;
	const string TYPE = "LABEL_DETECTION";
	const string MODEL = "builtin/latest";

	stringstream stream;

	for (auto data : encodings)
	{	
		// all c++ json implementations are a headache, therefore:
		stream << "\"{ \"requests\": [ { \"features\": [ { \"maxResults\": "
			<< MAX_RESULTS << ", \"type\": \"" << TYPE << "\", \"model\": \"" 
			<< MODEL << "\" } ], \"image\": { \"content\": \"" << data << "\" } } ] }\"";

		json_files.push_back(stream.str());

		// discard contents
		stream.str(string());
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
			else printf("conversion failure\n");
		}
}

int main(int argc, char** argv)
{	
	string api_key;
	vector<string> encodings, json_files;

	// validate(argv)

	load_key(argv[1], api_key);
	convert_segments(encodings, SEGMENT_PATH);
	generate_json(encodings, json_files);
	make_request(json_files, BAT_FILE, API, api_key);

	return EXIT_SUCCESS;
}
