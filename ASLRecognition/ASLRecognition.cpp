// ASLRecognition.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "opencv2/opencv.hpp"
#include <random>
#include "opencv2/xfeatures2d.hpp"

using namespace cv;
using namespace std;

char alphabet[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'K', 'L', 'M', 'N', 'O', 'P', 'V', 'W' };
char group0[] = { 'O'};
char group1[] = { 'B', 'D', 'F', 'G', 'L', 'M', 'N', 'P'};
char group2[] = { 'C', 'A', 'E', 'H', 'I', 'K', 'V', 'W' };

void readImages(Mat groupA[], Mat groupB[], Mat groupC[]) {
	//read group 0
	std::string name0;
	name0.append(1, group0[0]).append("asl.png");
	groupA[0] = imread(name0);
	//read group 1
	for (int i = 0; i < 8; i++) {
		std::string name1;
		name1.append(1, group1[i]).append("asl.png");
		groupB[i] = imread(name1);
	}
	//read group 2
	for (int i = 0; i < 8; i++) {
		std::string name2;
		name2.append(1, group2[i]).append("asl.png");
		groupC[i] = imread(name2);
	}
}

float innerAngle(float px1, float py1, float px2, float py2, float cx1, float cy1)
{

	float dist1 = std::sqrt((px1 - cx1)*(px1 - cx1) + (py1 - cy1)*(py1 - cy1));
	float dist2 = std::sqrt((px2 - cx1)*(px2 - cx1) + (py2 - cy1)*(py2 - cy1));

	float Ax, Ay;
	float Bx, By;
	float Cx, Cy;

	//find closest point to C
	//printf("dist = %lf %lf\n", dist1, dist2);

	Cx = cx1;
	Cy = cy1;
	if (dist1 < dist2)
	{
		Bx = px1;
		By = py1;
		Ax = px2;
		Ay = py2;
	}
	else {
		Bx = px2;
		By = py2;
		Ax = px1;
		Ay = py1;
	}


	float Q1 = Cx - Ax;
	float Q2 = Cy - Ay;
	float P1 = Bx - Ax;
	float P2 = By - Ay;


	float A = std::acos((P1*Q1 + P2 * Q2) / (std::sqrt(P1*P1 + P2 * P2) * std::sqrt(Q1*Q1 + Q2 * Q2)));

	A = A * 180 / CV_PI;

	return A;
}

int generateGestureRequest() {
	//int index = 0;
	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_int_distribution<> distr(0, 26);

	return distr(eng);
}

void performOpening(cv::Mat binaryImage, int kernelShape, cv::Point kernelSize) {
	cv::Mat structuringElement =
		getStructuringElement(kernelShape, kernelSize);

	morphologyEx(
		binaryImage, binaryImage, cv::MORPH_OPEN, structuringElement);
}


char gradeUserMovement(Mat& userGesture, Mat samples0[], Mat samples1[], Mat samples2[], std::vector<cv::Point>& fingers) {
	//0 = O
	//1 = B, D, F, G, L, M, N, P
	//2 = A, C, E, H, I, K, V, W
	//3 =
	//4 =
	//5 =
	char result;
	int numOfFingers = fingers.size();
	
	Mat descriptorGesture;
	std::vector<KeyPoint> keypointGesture;
	Ptr<Feature2D> detector = xfeatures2d::SIFT::create();
	detector->detect(userGesture, keypointGesture);
	detector->compute(userGesture, keypointGesture, descriptorGesture);
	//Ptr<Feature2D> extractor = xfeatures2d::SIFT::create();
	switch (numOfFingers) {
	case 0: {
		return 'O';
	} case 1: {
		
		int currentMatch = 0;
		char highestMatchLetter;
		for (int i = 0; i < 8; i++) {
			Mat descriptors1;
			std::vector<KeyPoint> keypoints1;
			vector<DMatch> matches;
			BFMatcher matcher;
			matcher.create(NORM_L2, true);
			Ptr<Feature2D> detector1 = xfeatures2d::SIFT::create();
			detector1->detect(samples1[i], keypoints1);
			
			detector1->compute(samples1[i], keypoints1, descriptors1);
			matcher.match(descriptorGesture, descriptors1, matches);
			
			if (matches.size() > currentMatch) {
				currentMatch = matches.size();
				highestMatchLetter = group1[i];
			}
		}
		return highestMatchLetter;
	} case 2: {
		
		int currentMatch = 0;
		char highestMatchLetter;
		for (int i = 0; i < 8; i++) {
			Mat descriptors2;
			std::vector<KeyPoint> keypoints2;
			vector<DMatch> matches;
			BFMatcher matcher;
			matcher.create(NORM_L2, true);
			Ptr<Feature2D> detector2 = xfeatures2d::SIFT::create();
			
			detector2->detect(samples2[i], keypoints2);

			detector2->compute(samples2[i], keypoints2, descriptors2);
			matcher.match(descriptorGesture, descriptors2, matches);
			

			if (matches.size() > currentMatch) {
				currentMatch = matches.size();
				highestMatchLetter = group2[i];
			}
		}
		return highestMatchLetter;
	} default:
		return '!';
	}
}

int main()
{
	VideoCapture camera;
	try {
		camera.open(0);
	}
	catch (exception& e) {
		cout << e.what() << '\n';
	}
	//read in the samples
	Mat samples0[1], samples1[8], samples2[8];
	readImages(samples0, samples1, samples2);


	//generate desired letter
	int letter = generateGestureRequest();
	char signLetter = alphabet[letter];

	while (camera.isOpened()) {
		Mat frame;
		camera.read(frame);

		Mat binaryHand(frame.rows, frame.cols, CV_8UC1);
		Mat greyScale;
		cvtColor(frame, greyScale, COLOR_BGR2HSV);

		//filtering out skin colour like regions
		for (int i = 0; i < frame.rows; i++) {
			for (int j = 0; j < frame.cols; j++) {
				int colorVal[3] = { 0,0,0 };
				colorVal[0] = frame.at<Vec3b>(i, j)[0];
				colorVal[1] = frame.at<Vec3b>(i, j)[1];
				colorVal[2] = frame.at<Vec3b>(i, j)[2];
				double ratio = colorVal[1] != 0 ? colorVal[2] / (double)colorVal[1] : colorVal[2];
				//printf("%f ", ratio);
				if (ratio > 1.05 && ratio < 3.8) {
					binaryHand.at<uchar>(i, j) = 255.0;
				}
				else {
					binaryHand.at<uchar>(i, j) = 0.0;
				}
			}
		}

		performOpening(binaryHand, cv::MORPH_ELLIPSE, { 3, 3 });
		dilate(binaryHand, binaryHand, cv::Mat(), cv::Point(-1, -1), 3);
		erode(binaryHand, binaryHand, cv::Mat(), cv::Point(-1, -1), 3);

		//finding the hand contour
		std::vector<std::vector<cv::Point> > contour;
		std::vector<Vec4i> hierarchy;
		findContours(binaryHand, contour, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);

		int biggest_contour_index = 0;
		double biggest_area = 0.0;
		//just interested in the biggest contour
		for (int i = 0; i < contour.size(); i++) {
			double area = contourArea(contour[i], false);
			if (area > biggest_area) {
				biggest_area = area;
				biggest_contour_index = i;
			}
		}
		//finding the convex hull
		std::vector<Point> hull_points;
		std::vector<int> hull_ints;
		convexHull(Mat(contour[biggest_contour_index]), hull_points, true);
		Rect bounding_rectangle = boundingRect(Mat(hull_points));
		Point center_bounding_rect(
			(bounding_rectangle.tl().x + bounding_rectangle.br().x) / 2,
			(bounding_rectangle.tl().y + bounding_rectangle.br().y) / 2
		);
		Scalar color = Scalar(140, 0, 255);
		
		//finding the convexity defect to select the two fingertips
		std::vector<int> hullIndexes;
		cv::convexHull(cv::Mat(contour[biggest_contour_index]), hullIndexes, true);
		std::vector<cv::Vec4i> convexityDefects;
		cv::convexityDefects(cv::Mat(contour[biggest_contour_index]), hullIndexes, convexityDefects);
		cv::Rect boundingBox = cv::boundingRect(hull_points);
		cv::rectangle(binaryHand, boundingBox, cv::Scalar(255, 0, 0));
		cv::Point center = cv::Point(boundingBox.x + boundingBox.width / 2, boundingBox.y + boundingBox.height / 2);
		std::vector<cv::Point> validPoints;
		//apply three thresholds here: inner angle, angle between center of the hand and starting point of
		//the convexity defect, and finally the length of the starting and middle point of the concave (defect)
		for (size_t i = 0; i < convexityDefects.size(); i++)
		{
			cv::Point p1 = contour[biggest_contour_index][convexityDefects[i][0]];//beginning
			cv::Point p2 = contour[biggest_contour_index][convexityDefects[i][1]];//end
			cv::Point p3 = contour[biggest_contour_index][convexityDefects[i][2]];//furthest
			double angle = std::atan2(center.y - p1.y, center.x - p1.x) * 180 / CV_PI;
			double inAngle = innerAngle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
			double length = std::sqrt(std::pow(p1.x - p3.x, 2) + std::pow(p1.y - p3.y, 2));
			if (angle > -30 && angle < 160 && std::abs(inAngle) > 20 && std::abs(inAngle) < 120 && length > 0.1 * boundingBox.height)
			{
				validPoints.push_back(p1);
			}
		}
		//draw out points that pass through all the filter (these are our fingertips)
		for (size_t i = 0; i < validPoints.size(); i++)
		{
			cv::circle(binaryHand, validPoints[i], 9, cv::Scalar(0, 255, 0), 2);
		}

		//drawContours(copy, contour, biggest_contour_index, color, 4);
		drawContours(binaryHand, contour, biggest_contour_index, color, 2, 8, hierarchy);
		polylines(binaryHand, hull_points, true, Scalar(255, 0, 0));
		//draw the center of the hand
		circle(binaryHand, center_bounding_rect, 5, Scalar(0, 0, 255), 2, 8);
		
		//print out the text
		string prints;
		prints.append(1, signLetter);
		putText(binaryHand, prints, Point(100,100), FONT_HERSHEY_DUPLEX, 5, color, 6);
		
		imshow("Camera View", binaryHand);
		char key = cv::waitKey(250);
		if (key == 'q') {
			return 0;
		}
		else if (key == 'g') {
			char result = gradeUserMovement(binaryHand, samples0, samples1, samples2, validPoints);
			std::cout << "You are doing the letter: " << result;
		}
	}
	cout << "Program started";
}

