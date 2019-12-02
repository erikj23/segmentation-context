
// 11 2019, Authors:
//	Jeremy Deng
//	Erik Maldonado

//opencv
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

//std
#include <cmath>
#include <String>
#include <iostream>
#include <direct.h> 

//namespace
using namespace cv;
using namespace std;


// assumptions:
//	filename: valid image file in working directory
//	cluster_size: integer values: [2-20]
// outcome: outputing the image into segments 
void segmentation(const string& filename, const int& cluster_size) {
	//read in the image 
	Mat input = imread(filename);
	Mat ocv = input.clone();

	//making output folder
	string file_dir = filename.substr(9, filename.substr(9).find("."));
	string create_dir = format("./segments/%s", file_dir.c_str());
	_mkdir(create_dir.c_str());

	// convert image pixel to float & reshape to a [3 x W*H] Mat 
	//  (so every pixel is on a row of it's own)
	Mat data;
	ocv.convertTo(data, CV_32F);
	data = data.reshape(1, data.total());

	// do kmeans
	Mat labels, centers;
	int clusters = cluster_size;
	kmeans(data, clusters, labels, TermCriteria(TermCriteria::EPS + TermCriteria::MAX_ITER, 10, 1.0), 10,
		KMEANS_RANDOM_CENTERS, centers);

	// reshape labels and cluster centers to a single row of Vec3f pixels:
	centers = centers.reshape(3, centers.rows);
	data = data.reshape(3, data.rows);

	//for each cluster, outputing the segments 
	for (int center_id = 0; center_id < clusters; center_id++) {
		Mat temp = data.clone();
		Vec3f* pointer = temp.ptr<Vec3f>();
		for (int i = 0; i < temp.rows; i++) {
			if (center_id == labels.at<int>(i)) {
				pointer[i] = centers.at<Vec3f>(center_id);
			}
			else {
				pointer[i] = Vec3f{ 0,0,0 };
			}
		}
		string output_dir = format("./segments/%s/%s_%d.jpg", file_dir.c_str(), file_dir.c_str(), center_id);
		temp = temp.reshape(3, ocv.rows);
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
	ocv = data.reshape(3, ocv.rows);
	ocv.convertTo(ocv, CV_8U);

	//display the k mean result and output into the segment directory
	namedWindow("Original Image");
	imshow("Original Image", ocv);
	waitKey(0);
	_mkdir(format("./output/%s", file_dir.c_str()).c_str());
	imwrite(format("./output/%s/%s_kmean.jpg", file_dir.c_str(), file_dir.c_str()), ocv);
}


int main(int argc, char* argv[])
{
	string filename = "./images/background.jpg";
	segmentation(filename, 8);

	return 0;
}
