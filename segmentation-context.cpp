
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
#include <locale>
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

// windows
#include <Windows.h>

// custom
#include "base64.h"

// namespaces
using namespace cv;
using namespace std;
using namespace web;

// constants
const filesystem::path INPUT_PATH("input");
const filesystem::path OUTPUT_PATH("output");
const filesystem::path SEGMENT_PATH("segments");

// globals
wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;

// assuptions:
// outcome:
string to_string(const wstring wide_string)
{
	
	return converter.to_bytes(wide_string);;
}

// assuptions:
// outcome:
wstring to_wstring(const string normal_string)
{
	return converter.from_bytes(normal_string);
}

// assuptions:
// outcome:
void load_key(const string& file_name, string& api_key)
{
	ifstream key_file(file_name);
	if (!getline(key_file, api_key)) printf("load key failure\n");
}

//todo change to use cpprest
// work in progress
// assuptions:
// outcome:
// improvements:
//	create executable class to retrieve output
void make_request(const vector<json::value>& json_objects, const string& api_key)
{
	json::value responses = json::value::object();	
	
	//cout << to_string(json_objects[1].serialize()) << endl;

	// setup uri
	uri_builder uri_path(L"https://vision.googleapis.com/");
	uri_path.append_path(L"v1/images:annotate");
	uri_path.append_query(L"key", to_wstring(api_key));
	
	// setup client
	http::client::http_client client(uri_path.to_uri());

	for (auto json_object : json_objects)
	{		
		// setup request
		http::http_request post(http::methods::POST);
		post.set_body(json_object);
		
		pplx::task<void> async_chain = client.request(post)
		.then([](http::http_response response)
		{
			printf("Received response status code:%u\n", response.status_code());
			cout << to_string(response.to_string()) << endl;
		});

		try { async_chain.wait(); }
		catch (const exception & e) { printf("Error exception:%s\n", e.what()); }
		//cout << to_string(uri_path.to_string()) << endl;

		break;
	}
	//http::client::http_client client(SCHEME_AUTHORITY);
	
	//uri_builder builder(U("/search"));
	//builder.append_query(U("q"), U("cpprestsdk github"));
	//return client.request(methods::GET, builder.to_string())
	//ofstream bat(file_name);
	//
	//for (const string& json_file: json_objects)
	//{
	//	bat << "curl --show-error --header \"Content-Type: application/json\" "
	//		<< "--request POST " << api << api_key << " --data @" << json_file << endl;
	//}
	//bat.close();
}

//todo change this to use cpprest
// work in progress
// assumptions:
// outcome:
// improvements:
//	trade constants for variables
//	failure handling
void generate_json(const vector<string>& encodings, vector<json::value>& json_objects)
{	
	const size_t MAX_RESULTS = 50;
	const wstring TYPE = L"LABEL_DETECTION";
	const wstring MODEL = L"builtin/latest";

	json::value requests;
	string json_string;

	for (auto data : encodings)
	{	
		requests = json::value::object();
		requests[L"requests"] = json::value::array();

		requests[L"requests"][0] = json::value::object();		
		requests[L"requests"][0][L"features"] = json::value::array();
		
		requests[L"requests"][0][L"features"][0] = json::value::object();
		requests[L"requests"][0][L"features"][0][L"maxResults"] = json::value(MAX_RESULTS);
		requests[L"requests"][0][L"features"][0][L"type"] = json::value(TYPE);
		requests[L"requests"][0][L"features"][0][L"model"] = json::value(MODEL);
		
		//requests[L"requests"][1] = json::value::object();
		requests[L"requests"][0][L"image"] = json::value::object();
		requests[L"requests"][0][L"image"][L"content"] = json::value(to_wstring(data));
		
		//json_string = to_string(requests.serialize().c_str());
		json_objects.push_back(requests);
		//printf("%s\n", json_string.c_str());
	}
}

// assumptions:
// outcome:
// improvements:
//	trade constant for variable
//	failure handling
void convert_segments(vector<string>& encodings, const filesystem::path& path)
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

//
int main(int argc, char** argv)
{	
	string api_key;
	vector<string> encodings;
	vector<json::value> json_objects;

	//validate(argv) add error handling for arguments
	load_key(argv[1], api_key);
	convert_segments(encodings, SEGMENT_PATH);
	generate_json(encodings, json_objects);
	make_request(json_objects, api_key);

	return EXIT_SUCCESS;
}
