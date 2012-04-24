#include <string>
#include <opencv2/opencv.hpp>

namespace ui
{

using std::string;
using cv::Mat;

bool has_display ();
void show (const string& name, Mat& frame);
int wait ();
double pref (const string &trackbarname, const string& winname,
	int maximum, double multiplier, double defvalue);

}
