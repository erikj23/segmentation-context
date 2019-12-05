
// 11 2019, Authors:
//	Jeremy Deng
//	Erik Maldonado

//std
#include <String>
#include <iostream>
#include <direct.h> 

//opencv
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

//namespace
using namespace cv;
using namespace std;


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
//	filename: valid image file in working directory
//	cluster_size: integer values: [2-20]
// outcome: outputing the image into segments 
void segmentation(const string& filename, const int& cluster_size) {
	//read in the image 
	Mat input = imread(filename);
	Mat ocv = input.clone();

	//get the proper name of this file 
	string file_dir = filename.substr(9, filename.substr(9).find("."));

	// convert image pixel to float & reshape to a [3 x W*H] Mat 
	//  (so every pixel is on a row of it's own)
	Mat data;
	ocv.convertTo(data, CV_32F);
	data = data.reshape(1, static_cast<int>(data.total()));

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
				//pointer[i] = centers.at<Vec3f>(center_id);
			}
			else {
				pointer[i] = Vec3f{ 0,0,0 };
			}
		}
		string output_dir = format("./segments/%s/%s_kmean_%d.jpg", file_dir.c_str(), file_dir.c_str(), center_id);
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
	//namedWindow("Original Image");
	//imshow("Original Image", ocv);
	imwrite(format("./segments/%s/%s_kmean.jpg", file_dir.c_str(), file_dir.c_str()), ocv);
}


int main(int argc, char* argv[])
{
	//assumption that the imgage
	string filename = "./images/background.jpg";
	string file_dir = filename.substr(9, filename.substr(9).find("."));
	string create_dir = format("./segments/%s", file_dir.c_str());

	//making the folder for output 
	auto t = _mkdir(format("./segments/%s", file_dir.c_str()).c_str());
	auto a = _mkdir(format("./output/%s", file_dir.c_str()).c_str());

	Mat img = imread(filename);
	segmentation(filename, 4);

	//cutting the image in five window
	Rect quadrant1(10, 10, img.cols / 2 - 10, img.rows / 2 - 10);
	Rect quadrant4(img.cols / 2 + 10, img.rows / 2 + 10, img.cols / 2 - 10, img.rows / 2 - 10);
	Rect quadrant3(10, img.rows / 2 + 10, img.cols / 2 - 10, img.rows / 2 - 10);
	Rect quadrant2(img.cols / 2 + 10, 10, img.cols / 2 - 10, img.rows / 2 - 10);

	//using the window located in the center and have a width of col*0.75, height of row*0.75
	Rect center(static_cast<int>(img.cols * 0.25 / 2), static_cast<int>(img.rows * 0.25 / 2), static_cast<int>(img.cols * 0.75), static_cast<int>(img.rows * 0.75));

	//initializing directory for each cut 
	string gc_outputfile_dir1 = format("./segments/%s/%s_gc_%s.jpg", file_dir.c_str(), file_dir.c_str(), "quadrant1");
	string gc_outputfile_dir2 = format("./segments/%s/%s_gc_%s.jpg", file_dir.c_str(), file_dir.c_str(), "quadrant2");
	string gc_outputfile_dir3 = format("./segments/%s/%s_gc_%s.jpg", file_dir.c_str(), file_dir.c_str(), "quadrant3");
	string gc_outputfile_dir4 = format("./segments/%s/%s_gc_%s.jpg", file_dir.c_str(), file_dir.c_str(), "quadrant4");
	string gc_outputfile_dir5 = format("./segments/%s/%s_gc_%s.jpg", file_dir.c_str(), file_dir.c_str(), "center");

	imwrite(gc_outputfile_dir1, _grabCut(img, quadrant1));
	imwrite(gc_outputfile_dir2, _grabCut(img, quadrant2));
	imwrite(gc_outputfile_dir3, _grabCut(img, quadrant3));
	imwrite(gc_outputfile_dir4, _grabCut(img, quadrant4));
	imwrite(gc_outputfile_dir5, _grabCut(img, center));

	return 0;
}
