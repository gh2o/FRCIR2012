#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstdio>
#include <csignal>

#define RGB(r,g,b) Scalar(b,g,r)
#define CAM true

using namespace std;
using namespace cv;

string outputWin = "Output";
string otherWin = "Other";

VideoWriter *writer = new VideoWriter ();
int poly = 0;
int thres = 0;

void topleft (vector<Point>& pts)
{
	int mini = 0;
	int minv = numeric_limits<int>::max ();
	for (int i = 0; i < pts.size (); i++)
	{
		Point& pt = pts[i];
		int v = pt.x + pt.y;
		if (v < minv)
		{
			mini = i;
			minv = v;
		}
	}
	
	if (mini > 0) 
		rotate (pts.begin (), pts.begin () + mini, pts.end ());
}

void process (Mat& frame, Mat& output, Mat &other)
{
	Mat gray, bin;
	cvtColor (frame, gray, CV_RGB2GRAY);
	compare (gray, thres, bin, CMP_GE);
	
	const int iterations = 4;
	dilate (bin, bin, Mat (), Point (-1, -1), iterations);
	erode (bin, bin, Mat (), Point (-1, -1), iterations);
	
	cvtColor (gray, output, CV_GRAY2RGB);
	other = bin;
	
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	{
		Mat fc = bin.clone ();
		findContours (fc, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_TC89_KCOS);
	}
	
	// find rectangles
	vector<bool> isrect (contours.size (), false);
	vector< vector<Point> > rect (contours.size ());
	
	allcontours:
	for (int i = 0; i < contours.size (); i++)
	{
		vector<Point> ctpts = contours[i];
		convexHull (ctpts, ctpts);
		
		double length = arcLength (ctpts, true);
//		if (length < 80)
//			continue;
		
		vector<Point> curve;
		approxPolyDP (ctpts, curve, length * poly * 0.001, true);
		
		if (
			curve.size () != 4 ||
			//contourArea (curve) < 220 ||
			!isContourConvex (curve)
		)
			continue;
		
		isrect[i] = true;
		rect[i] = curve;
	}
	
	// remove extremities
	for (int i = 0; i < rect.size (); i++)
	{
		if (!isrect[i]) // not a rectangle
			continue;
		
		vector<Point>& curve = rect[i];
		
		for (int j = 0; j < 4; j++)
		{
			Point& pt = curve[j];
			if (
				pt.x <= 1 || pt.x >= frame.cols - 2 ||
				pt.y <= 1 || pt.y >= frame.rows - 2
			)
			{
				isrect[i] = false;
				break;
			}
		}
	}
	
	// ensure inner rectangles
	for (int i = 0; i < rect.size (); i++)
	{
		if (!isrect[i]) // not a rectangle
			continue;
		
		int child = hierarchy[i][2];
		// if no inner contour or inner contour not a rectangle
		if (child < 0 || !isrect[child])
			isrect[i] = false;
	}
	
	// remove inner rectangles
	for (int i = 0; i < rect.size (); i++)
	{
		if (!isrect[i]) // not a rectangle
			continue;
			
		int cur = i;
		while ((cur = hierarchy[cur][3]) >= 0)
		{
			if (isrect[cur])
			{
				isrect[i] = false;
				break;
			}
		}
	}
	
	double dist = 0.0;
	
	for (int i = 0; i < rect.size (); i++)
	{
		if (!isrect[i])
			continue;
		
		vector<Point>& _ipts = rect[i];
		// rotate it so that first point is top-left
		topleft (_ipts);
		
		vector<Point2f> ipts (_ipts.size ());
		for (int j = 0; j < 4; j++)
			ipts[j] = _ipts[j];
		
		line (output, ipts[0], ipts[1], RGB (255, 0, 0));
		line (output, ipts[1], ipts[2], RGB (0, 255, 0));
		line (output, ipts[2], ipts[3], RGB (0, 0, 255));
		line (output, ipts[3], ipts[0], RGB (255, 255, 0));
		
		Point3f _wpts[] = {
			Point3f (-10, -7, 0),
			Point3f (10, -7, 0),
			Point3f (10, 7, 0),
			Point3f (-10, 7, 0),
		};
		vector<Point3f> wpts (_wpts, _wpts + sizeof (_wpts) / sizeof (Point3f));
		
		//cout << ipts << " -> " << wpts << endl;
		
		Mat rvec, tvec;
		solvePnP (
			wpts, ipts,
			getDefaultNewCameraMatrix (Mat::eye (3, 3, CV_64F) * 640, frame.size (), true), Mat (),
			rvec, tvec
		);
		
		Point pt = _ipts[0];
		
		char buf[1000];
		sprintf (buf, "%f,%f,%f", rvec.at<double> (0), rvec.at<double> (1), rvec.at<double> (2));
		putText (output, buf, Point (pt.x - 100, pt.y), FONT_HERSHEY_SIMPLEX, 0.4, RGB (0, 255, 0));
		sprintf (buf, "%f,%f,%f", tvec.at<double> (0), tvec.at<double> (1), tvec.at<double> (2));
		putText (output, buf, Point (pt.x - 100, pt.y + 20), FONT_HERSHEY_SIMPLEX, 0.4, RGB (0, 255, 0));
		
		dist = tvec.at<double> (2) * 0.1453;
		/*
		{
			double x = tvec.at<double> (0);
			double y = tvec.at<double> (1);
			double z = tvec.at<double> (2);
			dist = sqrt (x * x + y * y + z * z) / 640 / 640;
		}
		*/
	}
	
	{
		Scalar color = RGB (255, 0, 0);
		int barwidth = output.cols;
		
		if (dist != 0)
		{
			color = RGB (0, 255, 0);
			barwidth = (int)(output.cols * dist / 50);
		}
		
		rectangle (output, Point (0, 0), Point (barwidth, 20), color, CV_FILLED);
		
		if (dist != 0)
		{
			stringstream s;
			s << dist << "\"";
			putText (output, s.str (), Point (12, output.rows - 18),
				FONT_HERSHEY_SIMPLEX, 0.8, RGB (0, 255, 0)
			);
		}
	}
}

void savevideo (int sig)
{
	if (writer->isOpened ())
		delete writer;
	exit (0);
}

void prepsavevideo ()
{
	signal (SIGTERM, savevideo);
	signal (SIGINT, savevideo);
	signal (SIGHUP, savevideo);
}

int main (int argc, char *argv[])
{
	namedWindow (outputWin, CV_WINDOW_AUTOSIZE | CV_GUI_EXPANDED);
	namedWindow (otherWin, CV_WINDOW_AUTOSIZE | CV_GUI_EXPANDED);
	
	#if CAM
	VideoCapture capture (-1);
	Mat frame;
	
	stringstream fn;
	fn << "two" << time (NULL) << ".mov";
	writer->open (fn.str (), CV_FOURCC ('M', 'P', '4', 'V'), 24,
		Size (
			capture.get (CV_CAP_PROP_FRAME_WIDTH),
			capture.get (CV_CAP_PROP_FRAME_HEIGHT
		)
	));
	#else
	Mat frame = imread ("in.jpg", 1);
	resize (frame, frame, Size (640, 640 * frame.rows / frame.cols));
	#endif
	
	createTrackbar ("Poly", outputWin, &poly, 1000);
	setTrackbarPos ("Poly", outputWin, 20);
	createTrackbar ("Threshold", outputWin, &thres, 256);
	setTrackbarPos ("Threshold", outputWin, 224);
	
	prepsavevideo ();
	
	while (true)
	{
		#if CAM
		capture >> frame;
		#endif
		Mat output, other;
		process (frame, output, other);
		imshow (outputWin, output);
		imshow (otherWin, other);
		#if CAM
		*writer << output;
		#endif
		waitKey (1000 / 60);
	}
}
