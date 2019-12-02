
// 11 2019, Authors:
//	Jeremy Deng
//	Erik Maldonado

// codecvt deprecated in c++17 and up but std committee will not be removing codecvt for the 
// foreseable future until a replacedment is standardized
// source: https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

// std
#include <codecvt>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

// opencv
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

// vcpkg
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>

// custom
#include "base64.h"

// namespaces
using namespace cv;
using namespace std;
using namespace web;

// global constants
const filesystem::path INPUT_PATH("images");
const filesystem::path OUTPUT_PATH("output");
const filesystem::path SEGMENT_PATH("segments");

// global variables
wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;

// outcome: returns string converted from wstring
string to_string(const wstring wide_string)
{
	return converter.to_bytes(wide_string);
}

// outcome: returns wstring converted from string
wstring to_wstring(const string normal_string)
{
	return converter.from_bytes(normal_string);
}

// assuptions:
//	file_name: is a valid file in the working directory that contains a
//		working gcp api key for the vision api
// outcome: api_key is stores a valid gcp api key
void load_key(const string& file_name, string& api_key)
{
	ifstream key_file(file_name);

	if (!getline(key_file, api_key))
	{
		printf("load key failure\n");
		exit(EXIT_FAILURE);
	}
}

// assuptions:
//	json_objects: properly initialized vector containing valid json::value
//		objects that conform to the vision api request json schema 
//	api_key: string that contains valid gcp vision api key
//	group: properly initialized set 
// outcome:
//	group will be populated with all the label annotations associated with
//		with the api responses
void make_requests(const vector<json::value>& json_objects, const string& api_key, set<string>& group)
{	
	if (json_objects.empty()) return;

	string label;

	// setup uri
	uri_builder uri_path(L"https://vision.googleapis.com/");
	uri_path.append_path(L"v1/images:annotate");
	uri_path.append_query(L"key", to_wstring(api_key));
	
	// setup client
	http::client::http_client client(uri_path.to_uri());

	for (const auto& json_object: json_objects)
	{		
		// setup request
		http::http_request post(http::methods::POST);
		post.set_body(json_object);
		
		// async request
		pplx::task<void> async_chain = client.request(post)
		
		// handle http_response from client.request
		.then([&group, &label](http::http_response response)
		{	
			json::value result = response.extract_json().get();
			for (auto object : result[L"responses"][0][L"labelAnnotations"].as_array())
			{
				label = to_string(object[L"description"].serialize());
				label.erase(remove(label.begin(), label.end(), '\"'), label.end());
				group.insert(label);
			}
		});

		// wait for outstanding I/O
		try { async_chain.wait(); }
		catch (const exception & e) { printf("request exception:%s\n", e.what()); }
	}
}

// assumptions:
//	encodings: properly intialized vector of base64 encoded images
//	json_objects: properly intialized vector
// outcome: valid vision api requests created and stored in json_objects
// improvements:
//	trade constants for variables
//	more resilient failure handling
void generate_json(const vector<string>& encodings, vector<json::value>& json_objects)
{	
	if (encodings.empty()) return;

	const size_t MAX_RESULTS = 50;
	const wstring TYPE = L"LABEL_DETECTION";
	const wstring MODEL = L"builtin/latest";

	json::value requests;

	for (const auto& data: encodings)
	{	
		requests = json::value::object();
		requests[L"requests"] = json::value::array();

		requests[L"requests"][0] = json::value::object();		
		requests[L"requests"][0][L"features"] = json::value::array();

		requests[L"requests"][0][L"features"][0] = json::value::object();
		requests[L"requests"][0][L"features"][0][L"maxResults"] = json::value(MAX_RESULTS);
		requests[L"requests"][0][L"features"][0][L"type"] = json::value(TYPE);
		requests[L"requests"][0][L"features"][0][L"model"] = json::value(MODEL);

		requests[L"requests"][0][L"image"] = json::value::object();
		requests[L"requests"][0][L"image"][L"content"] = json::value(to_wstring(data));
		
		json_objects.push_back(requests);
	}
}

// assumptions:
//	path: valid path in the working directory that contains jpeg images grouped by directories
//	api_key: valid gcp vision api key
//	directory_labels: properly initialized map
// outcome: images in path are labeled and stored in directory_labels
// improvements:
//	trade constant for variable
//	more resilient failure handling
void label_images(const filesystem::path& path, const string& api_key, map<string, set<string>>& directory_labels)
{
	const string EXTENSION = ".jpg";

	Mat image;
	vector<uchar> buffer;
	vector<string> encodings;
	vector<json::value> json_objects;
	error_code e;
	
	// get first folder
	auto directory = filesystem::recursive_directory_iterator(path);
	string last_directory = directory->path().filename().string();
	directory.increment(e);

	// convert each jpeg image found in path to base64
	for (const auto& entry: directory)
	{
		if (entry.is_directory())
		{
			generate_json(encodings, json_objects);
			encodings.clear();

			directory_labels[last_directory] = set<string>();
			make_requests(json_objects, api_key, directory_labels[last_directory]);
			json_objects.clear();

			last_directory = entry.path().filename().string();
		}
		if (entry.is_regular_file())
		{	
			image = imread(entry.path().string());
			buffer.resize(static_cast<size_t>(image.rows) * static_cast<size_t>(image.cols));

			if (imencode(EXTENSION, image, buffer)) encodings.push_back(base64_encode(buffer.data(), buffer.size()));
			else printf("conversion failure\n");
		}
	}
	
	// identify any remaining images
	if (!encodings.empty())
	{
		generate_json(encodings, json_objects);
		encodings.clear();

		directory_labels[last_directory] = set<string>();
		make_requests(json_objects, api_key, directory_labels[last_directory]);
		json_objects.clear();
	}
}

// assumptions:
//	path: valid path in working directory
//	directory_labels: properly initialized map that contains string to set mappings of an image
//	name: non-empty string
// outcome: directory_label mappings written to disk specified by path
void write_json(const filesystem::path& path, const map<string, set<string>>& directory_labels, const string name)
{	
	int index;
	json::value data;

	// each iteration of key is one json object to written to disk
	for (const auto& [key, value]: directory_labels)
	{	
		data = json::value::object();
		data[to_wstring(key)] = json::value::array();
		
		index = 0;

		for (const auto& label: value) data[to_wstring(key)][index++] = json::value(to_wstring(label));

		// create directory with this key name, fails if already exists		
		filesystem::create_directory(path.string() + "\\" + key);

		// write file to directory
		ofstream json_file(path.string() + "\\" + key + "\\" + name + ".json");
		json_file << to_string(data.serialize());
		json_file.close();
	}
}

// outcomes: 
//	loads api key from disk
//	labels all images in segments directory
//	writes labels as json files to disk in output directory
int main(int argc, char** argv)
{	
	string api_key;
	map<string, set<string>> directory_labels;

	//validate(argv) add error handling for arguments
	load_key(argv[1], api_key);
	label_images(SEGMENT_PATH, api_key, directory_labels);
	write_json(OUTPUT_PATH, directory_labels, "segments");

	return EXIT_SUCCESS;
}
