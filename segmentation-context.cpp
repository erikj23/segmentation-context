
// 11 2019, Authors:
//	Jeremy Deng
//	Erik Maldonado

// NOTE: Project Properties assume OpenCV is located in:
//	C:\Program Files\OpenCV\
//		README.md.txt
//		LICENSE_FFMPEG.txt
//		LICENSE.txt
//		sources\
//		build\
//
//	To Change:
//		Solution Explorer > (right-click)segmentation-context > Properties
//			C/C++ > General > Additional Include Directories
//			Linker > General > Additional Library Directories

// codecvt deprecated in c++17 and up but std committee will not be removing codecvt for the 
// foreseable future until a replacement is standardized
// source: https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

// std
#include <algorithm>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <direct.h>
#include <stdio.h>

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
const double EPSILON = 1.0;
const int ATTEMPTS = 2;
const int ITER = 10;

// global variables
wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;

// assumptions:
//	img: valid image matrix in opencv
//	rectangle: rectangle are smaller than img window 
// outcome: outputing image with grabcuts segments  
Mat _grabCut(const Mat& img, Rect rectangle) {
	//initializing the matrix for grabcut
	Mat results;
	Mat background_m, foreground_m;

	//grab cut
	grabCut(img, results, rectangle, background_m, foreground_m, 1, GC_INIT_WITH_RECT);
	compare(results, GC_PR_FGD, results, CMP_EQ);

	//making the foreground objects 
	Mat foreground(img.size(), CV_8UC3, Scalar(0, 0, 0));
	img.copyTo(foreground, results);

	return foreground;
}

// assumptions:
//	input: valid image file loaded in opencv
//  file_dir: correct directory of the image 
//	cluster_size: integer values: [2-20]
// outcome: outputing the image into segments and write to disk
void segmentation(Mat& input, const string& file_dir, const int& cluster_size) {
	//read in the image and make a copy
	Mat img = input.clone();
	// convert image pixel to float & reshape to a [3 x W*H] Mat 
	//  (so every pixel is on a row of it's own)
	Mat data;
	img.convertTo(data, CV_32F);
	data = data.reshape(1, static_cast<int>(data.total()));
	
	// do kmeans
	Mat labels, centers;
	int clusters = cluster_size;
	kmeans(data, clusters, labels, TermCriteria(TermCriteria::EPS + TermCriteria::MAX_ITER, ITER, EPSILON), ATTEMPTS,
		KMEANS_RANDOM_CENTERS, centers);

	// reshape labels and cluster centers to a single row of Vec3f pixels
	centers = centers.reshape(3, centers.rows);
	data = data.reshape(3, data.rows);

	//for each cluster, outputing the segments 
	for (int center_id = 0; center_id < clusters; center_id++) {
		Mat temp = data.clone();
		Vec3f* pointer = temp.ptr<Vec3f>();
		for (int i = 0; i < temp.rows; i++) {
			if (center_id == labels.at<int>(i)) {
				//pointer[i] = centers.at<Vec3f>(center_id);
			}
			else {
				pointer[i] = Vec3f{ 0, 0, 0 };
			}
		}
		string output_dir = format("./segments/%s/%s_kmean%d_%d.jpg", file_dir.c_str(), file_dir.c_str(), clusters, center_id);
		temp = temp.reshape(3, img.rows);
		temp.convertTo(temp, CV_8U);
		imwrite(output_dir, temp);
	}

	// replace pixel values with their center value:
	Vec3f* p = data.ptr<Vec3f>();
	for (int i = 0; i < data.rows; i++) {
		int center_id = labels.at<int>(i);
		p[i] = centers.at<Vec3f>(center_id);
	}

	// back to 2d, and uchar:
	img = data.reshape(3, img.rows);
	img.convertTo(img, CV_8U);

	//display the k mean result and output into the segment directory
	//namedWindow("Original Image");
	//imshow("Original Image", ocv);
	imwrite(format("./segments/%s/%s_kmean%d_full.jpg", file_dir.c_str(), file_dir.c_str(), clusters), img);
}

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
// outcome: api_key is stores a valid gcp api key or program exits if key cannot be read
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
	
	// setup api
	http::client::http_client api(uri_path.to_uri());

	for (const auto& json_object: json_objects)
	{		
		// setup request
		http::http_request post(http::methods::POST);
		post.set_body(json_object);
		
		// async request
		pplx::task<void> async_chain = api.request(post)
		
		// handle http_response from api.request
		.then([&group, &label](http::http_response response)
		{	
			json::value result = response.extract_json().get();

			if (!result[L"responses"][0].as_object().size() == 0)
				for (auto object : result[L"responses"][0][L"labelAnnotations"].as_array())
				{	
					label = to_string(object[L"description"].serialize());
					label.erase(remove(label.begin(), label.end(), '\"'), label.end());
					group.insert(label);
				}
		});

		// wait for outstanding I/O
		try { async_chain.wait(); }
		catch (const exception& e) { printf("request exception:%s\n", e.what()); }
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

// assumptions:
//	argv[1]: valid api key for cloud vision api
//	argv[2]: file name for input image that exists in images directory
//	argv[3]: cluster size for kmeans algorithm [2-20]
// outcomes: 
//	loads api key from disk
//	segments input image
//	labels all images in segments directory
//	writes labels as json files to disk in output directory
int main(int argc, char** argv)
{	
	int cluster_size;
	char key_buffer[256], image_buffer[256];
	string arguments, api_key;

	// parse arguments
	for_each(argv + 1, argv + argc, [&arguments](const char* c_str) { arguments += string(c_str) + " ";	});
	sscanf_s(arguments.c_str(), "%s %s %d", key_buffer, static_cast<uint>(sizeof(key_buffer)), 
		image_buffer, static_cast<uint>(sizeof(image_buffer)), &cluster_size);
	
	// load api key or fail
	load_key(string(key_buffer), api_key);

	//assumption that the imgage
	string image_path = format("./images/%s/%s.jpg", image_buffer, image_buffer);	

	//making the folder for output 
	auto t = _mkdir(format("./segments/%s", image_buffer).c_str());
	
	Mat img = imread(image_path);
	segmentation(img, string(image_buffer), cluster_size);
	
	//cutting the image in five window
	Rect quadrant1(10, 10, img.cols / 2 - 10, img.rows / 2 - 10);
	Rect quadrant4(img.cols / 2 + 10, img.rows / 2 + 10, img.cols / 2 - 10, img.rows / 2 - 10);
	Rect quadrant3(10, img.rows / 2 + 10, img.cols / 2 - 10, img.rows / 2 - 10);
	Rect quadrant2(img.cols / 2 + 10, 10, img.cols / 2 - 10, img.rows / 2 - 10);
	
	//using the window located in the center and have a width of col*0.75, height of row*0.75
	Rect center(static_cast<int>(img.cols * 0.25 / 2), static_cast<int>(img.rows * 0.25 / 2), 
		static_cast<int>(img.cols * 0.75), static_cast<int>(img.rows * 0.75));
	
	//initializing directory for each cut 
	string gc_outputfile_dir1 = format("./segments/%s/%s_gc_%s.jpg", image_buffer, image_buffer, "quadrant1");
	string gc_outputfile_dir2 = format("./segments/%s/%s_gc_%s.jpg", image_buffer, image_buffer, "quadrant2");
	string gc_outputfile_dir3 = format("./segments/%s/%s_gc_%s.jpg", image_buffer, image_buffer, "quadrant3");
	string gc_outputfile_dir4 = format("./segments/%s/%s_gc_%s.jpg", image_buffer, image_buffer, "quadrant4");
	string gc_outputfile_dir5 = format("./segments/%s/%s_gc_%s.jpg", image_buffer, image_buffer, "center");
	
	imwrite(gc_outputfile_dir1, _grabCut(img, quadrant1));
	imwrite(gc_outputfile_dir2, _grabCut(img, quadrant2));
	imwrite(gc_outputfile_dir3, _grabCut(img, quadrant3));
	imwrite(gc_outputfile_dir4, _grabCut(img, quadrant4));
	imwrite(gc_outputfile_dir5, _grabCut(img, center));

	
	map<string, set<string>> directory_labels;
	
	label_images(INPUT_PATH, api_key, directory_labels);
	write_json(OUTPUT_PATH, directory_labels, "base_labels");
	directory_labels.clear();

	label_images(SEGMENT_PATH, api_key, directory_labels);
	write_json(OUTPUT_PATH, directory_labels, "segment_labels");
	directory_labels.clear();

	return EXIT_SUCCESS;
}
