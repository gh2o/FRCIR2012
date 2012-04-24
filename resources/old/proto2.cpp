#include <opencv2/opencv.hpp>

#define _STR(x) #x
#define STR(x) _STR(x)
#define SHOW(x) imshow (STR (__LINE__) " - " #x, x)

using namespace std;
using namespace cv;

void process (Mat& frame, Mat& output, Mat& other)
{
	resize (frame, frame, Size (320, 240));
	SHOW (frame);
	
	Mat red (frame.rows, frame.cols, CV_8UC1);
	Mat green (frame.rows, frame.cols, CV_8UC1);
	Mat blue (frame.rows, frame.cols, CV_8UC1);
	
	mixChannels (
		&frame, 1,
		(Mat[]){blue, green, red}, 3,
		(int[]){0,0,1,1,2,2}, 3
	);
	
	Mat filt;
	compare (green, 192, filt, CMP_GE);
	
	Mat diff;
	absdiff (red, blue, diff);
	
	Mat good;
	compare (diff, green * 0.9, good, CMP_LT);
	
	SHOW (good);
	SHOW (filt);
	bitwise_and (good, filt, filt);
	SHOW (filt);
	
	/*
	Mat hls;
	cvtColor (frame, hls, CV_BGR2HLS);
	
	Mat hue (hls.rows, hls.cols, CV_8UC1);
	Mat lum (hls.rows, hls.cols, CV_8UC1);
	Mat sat (hls.rows, hls.cols, CV_8UC1);
	Mat tmp;
	
	int fromto[] = {0,0,1,1,2,2};
	mixChannels (
		&hls, 1,
		(Mat[]){hue,lum,sat}, 3,
		(int[]){0,0,1,1,2,2}, 3
	);
	
	Mat filt1;
	compare (lum, 60, filt1, CMP_GE);
	
	Mat filt2;
	compare (hue, 60 - 20, tmp, CMP_GT);
	filt2 = tmp.clone ();
	compare (hue, 60 + 20, tmp, CMP_LT);
	bitwise_and (tmp, filt2, filt2);
	compare (lum, 96, tmp, CMP_GE);
	bitwise_and (tmp, filt2, filt2);
	
	Mat filt (hls.rows, hls.cols, CV_8UC1, Scalar (0));
	bitwise_or (filt1, filt, filt);
	bitwise_or (filt2, filt, filt);
	
	SHOW (filt1);
	SHOW (filt2);
	SHOW (filt);
	*/
	
	/*
	Mat fhue1, fhue2, flum, fsat;
	compare (channels[0], 120 - 60, fhue1, CMP_GT);
	compare (channels[0], 120 + 60, fhue2, CMP_LT);
	compare (channels[1], 128, flum, CMP_GE);
	compare (channels[2], 128, fsat, CMP_GE);
	
	imshow ("HUE1", fhue1);
	imshow ("HUE2", fhue2);
	imshow ("LUM", flum);
	imshow ("SAT", fsat);
	*/
	
	/*
	Mat channels[3];
	for (int i = 0; i < 3; i++)
		channels[i] = Mat (frame.rows, frame.cols, CV_8UC1);
	
	int fromto[] = {2,0,1,1,0,2};
	mixChannels (&frame, 1, channels, 3, fromto, 3);
	
	imshow ("R", channels[0]);
	imshow ("G", channels[1]);
	imshow ("B", channels[2]);
	
	Mat target (frame.rows, frame.cols, CV_8UC1, Scalar (255));
	for (int i = 0; i < 3; i++)
		min (channels[i], target, target);
	
	imshow ("MIN", target);
	*/
}

int main (int argc, char *argv[])
{
	VideoCapture capture (-1);
	Mat frame;
	
	while (true)
	{
		capture >> frame;
		Mat output, other;
		process (frame, output, other);
//		imshow ("Output", output);
//		imshow ("Other", other);
		waitKey (10);
	}
}

