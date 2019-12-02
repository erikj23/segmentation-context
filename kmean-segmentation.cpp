
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <string>
#include <iostream>
#include <direct.h> 
using namespace cv;


void k_mean (String filename, int cluster_size) {
	Mat ocv = imread(filename);
	String file_dir = filename.substr(8, filename.substr(8).find("."));
	String create_dir = format("./Segmentation/%s", file_dir.c_str());
	_mkdir(create_dir.c_str());

	// convert to float & reshape to a [3 x W*H] Mat 
	//  (so every pixel is on a row of it's own)
	Mat data;
	ocv.convertTo(data, CV_32F);
	data = data.reshape(1, data.total());

	// do kmeans
	Mat labels, centers;
	int clusters = cluster_size;
	kmeans(data, clusters, labels, TermCriteria(TermCriteria::EPS + TermCriteria::MAX_ITER, 10, 1.0), 10,
		KMEANS_RANDOM_CENTERS, centers);

	// reshape both to a single row of Vec3f pixels:
	centers = centers.reshape(3, centers.rows);
	data = data.reshape(3, data.rows);

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
		String output_dir = format("./Segmentation/%s/%s_%d.jpg", file_dir.c_str(), file_dir.c_str(), center_id);
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

	namedWindow("Original Image");
	imshow("Original Image", ocv);
	waitKey(0);
	imwrite(format("./Segmentation/%s/%s_full.jpg", file_dir.c_str(), file_dir.c_str()), ocv);
}


int main(int argc, char* argv[])
{
	String filename = "./input/background.jpg";
	k_mean (filename, 8);

	return 0;
}
